#include "adcs2.hpp"

ADCS2::ADCS2(std::string host_ip, int port) {
    m_host.Set_IP_Address(host_ip);
    m_host.Set_Port(port);
}

ADCS2::~ADCS2() {
    stop_collection();
}

void ADCS2::trigger () {
    m_recv_valid.store(false, std::memory_order_release);
    m_host.Send_Command(trig_cmd);

    // Launch async task to wait for 1 second
    // if we do not receive data by then, we
    // send another command called adc_data_all
    std::thread([this]() {
        // if m_recv_valid becomes true, we can stop waiting
        // but if it is still false after 1 second, we send the adc_data_all command to prompt the ADC to resend the data
        
        // Check every 50ms for up to 1 second
        for (int i = 0; i < 20; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (m_recv_valid.load(std::memory_order_acquire)) {
                return; // Data received, exit early
            }
        }

        std::cout << "INFO: [ADCS2::trigger] No data received after 1 second, sending adc_data_all command to prompt ADC to resend data.\n";
        
        // After 1 second, if still no data, send the command
        if (!m_recv_valid.load(std::memory_order_acquire)) {
            m_host.Send_Command("get adc_data_all");
        }
    }).detach();
}

void ADCS2::stop_collection() {
    // Stop collecting first
    collecting = false;

    // Send disconnect command
    m_host.Send_Command(disconnect_cmd);
    
    // Kill the server to unblock the recv() call in Receive_Get_Response()
    if (m_host.Is_Connected()) {
        m_host.Stop_Server();
    }

    // Now the thread can exit cleanly
    if (collection_thread.joinable()) {
        collection_thread.join();
    }
}

void ADCS2::spin_up_collection() {
    collecting = true;
    collection_thread = std::thread([this]() {
        while (collecting) {
            // Wait for response from the trigger command (adc_start_w)
            // No need to send commands - just receive the data
            ADCS2_CResponse response = Receive_Response();
            
            // If we're no longer collecting, exit immediately
            if (!collecting) break;
            
            if (!response.values.empty() && response.type == 1) { // Check if it's a get response with values
                // Enqueue the received ADC data for processing
                adc_data_queue.enqueue(response.values);
            }
        }
    });
}

void ADCS2::start() {
    m_host.Start_Server();

    ADCS2_CResponse response;
    // Setup parameters
    Send_Command("set", "adc_delay", {delay});
    response = Receive_Response();
    if (response.type == 0 && !response.success) {
        std::cerr << "Failed to set adc_delay" << std::endl;
    }

    Send_Command("set", "adc_average_n", {average_n});
    response = Receive_Response();
    if (response.type == 0 && !response.success) {
        std::cerr << "Failed to set adc_average_n" << std::endl;
    }

    Send_Command("set", "adc_burst_limit", {burst_n});
    response = Receive_Response();
    if (response.type == 0 && !response.success) {
        std::cerr << "Failed to set adc_burst_limit" << std::endl;
    }
}

void ADCS2::close() {
    stop_collection();
}

double ADCS2::get_delay () {
    Send_Command("get", "adc_delay", {});
    ADCS2_CResponse response = Receive_Response();
    if (response.type == 1 && !response.values.empty()) {
        return static_cast<double>(response.values[0]) * 8 / 1000; // Convert back to microseconds
    }
    std::cerr << "Failed to get adc_delay" << std::endl;
    return -1;
}

void ADCS2::set_delay (double delay_us) {
    // Calculate the delay in terms of
    // clock cycles

    // Each clock is 8 ns
    delay = static_cast<int>(delay_us * 1000 / 8);
}

int ADCS2::get_average_n () {
    Send_Command("get", "adc_average_n", {});
    ADCS2_CResponse response = Receive_Response();
    if (response.type == 1 && !response.values.empty()) {
        return static_cast<int>(response.values[0]);
    }
    std::cerr << "Failed to get adc_average_n" << std::endl;
    return -1;
}

void ADCS2::set_average_n (int n) {
    average_n = n;
}

void ADCS2::set_burst_n (int n) {
    burst_n = n;
}

int ADCS2::get_burst_n () {
    Send_Command("get", "adc_burst_limit", {});
    ADCS2_CResponse response = Receive_Response();
    if (response.type == 1 && !response.values.empty()) {
        return static_cast<int>(response.values[0]);
    }
    std::cerr << "Failed to get adc_burst_limit" << std::endl;
    return -1;
}

void ADCS2::reset() {
    Send_Command("set", "adc_reset", {1});
    ADCS2_CResponse response = Receive_Response();
    if (response.type == 0 && !response.success) {
        std::cerr << "Failed to reset ADC" << std::endl;
    }

    // Wait for 100 ms to allow the ADC to reset
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Send_Command("set", "adc_reset", {0});
    response = Receive_Response();
    if (response.type == 0 && !response.success) {
        std::cerr << "Failed to release ADC reset" << std::endl;
    }
}

std::string ADCS2::combine_params (std::string command, std::string app_command, std::vector<int> params) {
    std::string full_command = command + " " + app_command;
    for (const auto& param : params) {
        full_command += " " + std::to_string(param);
    }
    return full_command;
}

std::string ADCS2::clean_commands (std::string command) {
    // Remove extra spaces and convert to lowercase
    std::string cleaned;
    bool in_space = false;
    for (char c : command) {
        if (std::isspace(c)) {
            if (!in_space) {
                cleaned += ' ';
                in_space = true;
            }
        } else {
            cleaned += std::tolower(c);
            in_space = false;
        }
    }
    // Trim leading/trailing spaces
    size_t start = cleaned.find_first_not_of(' ');
    size_t end = cleaned.find_last_not_of(' ');
    if (start == std::string::npos) return ""; // All spaces
    return cleaned.substr(start, end - start + 1);
}

void ADCS2::Send_Command (std::string command, std::string app_command, std::vector<int> params) {
    std::string full_command = combine_params(command, app_command, params);
    std::string cleaned_command = clean_commands(full_command);
    m_host.Send_Command(cleaned_command);
}

// Response receiving methods
bool ADCS2::try_get_data(std::vector<uint32_t>& data) {
    bool result = adc_data_queue.try_dequeue(data);
    if (result) {
        m_recv_valid.store(true, std::memory_order_release);
    }
    return result;
}

bool ADCS2::has_data() const {
    return adc_data_queue.size_approx() > 0;
}

size_t ADCS2::queue_size() const {
    return adc_data_queue.size_approx();
}

void ADCS2::clear_queue() {
    std::vector<uint32_t> dummy;
    while (adc_data_queue.try_dequeue(dummy)) {
        // Just drain the queue
    }
}

bool ADCS2::Is_Connected() const {
    return m_host.Is_Connected();
}

ADCS2_CResponse ADCS2::Receive_Response() {
    ADCS2_CResponse response;
 
    ADCHost::ResponseType type = m_host.Get_Response_Type();

    switch (type) {
        case ADCHost::ResponseType::SET: {
            SetResponse set_response = m_host.Receive_Set_Response();
            response.type = 0;
            response.success = set_response.success;
            break;
        }
        case ADCHost::ResponseType::GET: {
            GetResponse get_response = m_host.Receive_Get_Response();
            response.type = 1;
            response.values = get_response.values;
            break;
        }
        case ADCHost::ResponseType::HELP: {
            HelpResponse help_response = m_host.Receive_Help_Response();
            response.type = 2;
            response.message = help_response.message;
            break;
        }
        default:
            response.type = -1; // Unknown type
            break;
    }
    return response;
}

void ADCS2::Set_IP_Address(const std::string& ip) {
    m_host.Set_IP_Address(ip);
}

void ADCS2::Set_Port(int port) {
    m_host.Set_Port(port);
}

bool ADCS2::recv_valid() const {
    return m_recv_valid.load(std::memory_order_acquire);
}