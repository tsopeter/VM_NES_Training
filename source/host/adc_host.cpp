#include "adc_host.hpp"

ADCHost::ADCHost () {

}

ADCHost::~ADCHost () {
    if (m_server_running) {
        Stop_Server();
    }
}

void ADCHost::Set_IP_Address (std::string ip_address) {
    m_ip_address = ip_address;
}

void ADCHost::Set_Port (int port) {
    m_port = port;
}

void ADCHost::Start_Server () {
    if (m_server_running) {
        std::cerr << "[HOST] Server is already running." << std::endl;
        return;
    }

    // Create socket
    m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_socket < 0) {
        std::cerr << "[HOST] Error creating socket: " << strerror(errno) << std::endl;
        return;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "[HOST] Error setting SO_REUSEADDR: " << strerror(errno) << std::endl;
        close(m_server_socket);
        return;
    }

    if (setsockopt(m_server_socket, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
        std::cerr << "[HOST] Error setting TCP_NODELAY: " << strerror(errno) << std::endl;
        close(m_server_socket);
        return;
    }

    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(m_port);
    
    if (m_ip_address == "0.0.0.0" || m_ip_address == "") {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, m_ip_address.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "[HOST] Invalid IP address: " << m_ip_address << std::endl;
            close(m_server_socket);
            return;
        }
    }

    if (bind(m_server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[HOST] Error binding socket: " << strerror(errno) << std::endl;
        close(m_server_socket);
        return;
    }

    // Listen
    if (listen(m_server_socket, 1) < 0) {
        std::cerr << "[HOST] Error listening: " << strerror(errno) << std::endl;
        close(m_server_socket);
        return;
    }

    m_server_running = true;
    std::cout << "[HOST] Listening on " << m_ip_address << ":" << m_port << "..." << std::endl;

    // Start server thread
    m_server_thread = std::thread(&ADCHost::Server_Thread, this);

    // Block until client connects
    while (!m_client_connected) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ADCHost::Stop_Server () {
    if (!m_server_running) {
        std::cerr << "[HOST] Server is not running." << std::endl;
        return;
    }

    m_server_running = false;

    // Close client socket if connected
    if (m_client_socket >= 0) {
        close(m_client_socket);
        m_client_socket = -1;
    }

    // Close server socket
    if (m_server_socket >= 0) {
        close(m_server_socket);
        m_server_socket = -1;
    }

    // Wait for server thread to finish
    if (m_server_thread.joinable()) {
        m_server_thread.join();
    }

    m_client_connected = false;
    std::cout << "[HOST] Server stopped." << std::endl;
}

void ADCHost::Server_Thread () {
    while (m_server_running) {
        // Accept connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        m_client_socket = accept(m_server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (m_client_socket < 0) {
            if (m_server_running) {
                std::cerr << "[HOST] Error accepting connection: " << strerror(errno) << std::endl;
            }
            break;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        std::cout << "[HOST] Connection established with " << client_ip << ":" 
                  << ntohs(client_addr.sin_port) << std::endl;
        
        m_client_connected = true;

        // Keep connection alive until client disconnects or server stops
        while (m_server_running && m_client_connected) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

bool ADCHost::Send_Command (const std::string& command) {
    if (!m_client_connected || m_client_socket < 0) {
        std::cerr << "[HOST] Cannot send command: not connected" << std::endl;
        return false;
    }

    ssize_t sent = send(m_client_socket, command.c_str(), command.length(), 0);
    if (sent < 0) {
        std::cerr << "[HOST] Error sending command: " << strerror(errno) << std::endl;
        m_client_connected = false;
        return false;
    }

    return true;
}

std::vector<uint8_t> ADCHost::Receive_Raw (size_t buffer_size) {
    std::vector<uint8_t> buffer(buffer_size);
    
    ssize_t received = recv(m_client_socket, buffer.data(), buffer_size, 0);
    if (received < 0) {
        std::cerr << "[HOST] Error receiving data: " << strerror(errno) << std::endl;
        m_client_connected = false;
        return {};
    }
    
    if (received == 0) {
        std::cout << "[HOST] Client disconnected" << std::endl;
        m_client_connected = false;
        return {};
    }

    buffer.resize(received);
    return buffer;
}

uint32_t ADCHost::Decode_U32_Network_Byte_Order (const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
           static_cast<uint32_t>(data[3]);
}

ADCHost::ResponseType ADCHost::Get_Response_Type () {
    // Peek at the first 4 bytes to get command type
    uint8_t buffer[4];
    ssize_t received = recv(m_client_socket, buffer, 4, MSG_PEEK);
    
    if (received < 4) {
        return ResponseType::UNKNOWN;
    }

    uint32_t command_type = Decode_U32_Network_Byte_Order(buffer);
    
    switch (command_type) {
        case 1: return ResponseType::SET;
        case 2: return ResponseType::GET;
        case 3: return ResponseType::HELP;
        default: return ResponseType::UNKNOWN;
    }
}

SetResponse ADCHost::Receive_Set_Response () {
    SetResponse response;
    response.success = false;

    auto data = Receive_Raw(8);
    if (data.size() < 8) {
        std::cerr << "[HOST] Invalid set response: expected at least 8 bytes, got " 
                  << data.size() << std::endl;
        return response;
    }

    uint32_t command_type = Decode_U32_Network_Byte_Order(&data[0]);
    uint32_t status = Decode_U32_Network_Byte_Order(&data[4]);

    if (command_type != 1) {
        std::cerr << "[HOST] Expected set response (type 1), got type " << command_type << std::endl;
        return response;
    }

    response.success = (status == 1);
    return response;
}

GetResponse ADCHost::Receive_Get_Response () {
    GetResponse response;

    auto data = Receive_Raw(4096);
    if (data.size() < 8) {
        std::cerr << "[HOST] Invalid get response: expected at least 8 bytes, got " 
                  << data.size() << std::endl;
        return response;
    }

    uint32_t command_type = Decode_U32_Network_Byte_Order(&data[0]);
    uint32_t n_values = Decode_U32_Network_Byte_Order(&data[4]);

    if (command_type != 2) {
        std::cerr << "[HOST] Expected get response (type 2), got type " << command_type << std::endl;
        return response;
    }

    if (data.size() < 8 + n_values * 4) {
        std::cerr << "[HOST] Invalid get response: expected " << (8 + n_values * 4) 
                  << " bytes, got " << data.size() << std::endl;
        return response;
    }

    response.values.reserve(n_values);
    for (uint32_t i = 0; i < n_values; ++i) {
        uint32_t value = Decode_U32_Network_Byte_Order(&data[8 + i * 4]);
        response.values.push_back(value);
    }

    return response;
}

HelpResponse ADCHost::Receive_Help_Response () {
    HelpResponse response;

    auto data = Receive_Raw(4096);
    if (data.size() < 4) {
        std::cerr << "[HOST] Invalid help response: expected at least 4 bytes, got " 
                  << data.size() << std::endl;
        return response;
    }

    uint32_t command_type = Decode_U32_Network_Byte_Order(&data[0]);

    if (command_type != 3) {
        std::cerr << "[HOST] Expected help response (type 3), got type " << command_type << std::endl;
        return response;
    }

    // The rest is UTF-8 text
    response.message = std::string(data.begin() + 4, data.end());
    return response;
}

bool ADCHost::Send_Set_Command (const std::string& app_command, const std::vector<std::string>& params) {
    std::string command = "set " + app_command;
    for (const auto& param : params) {
        command += " " + param;
    }

    if (!Send_Command(command)) {
        return false;
    }

    SetResponse response = Receive_Set_Response();
    return response.success;
}

GetResponse ADCHost::Send_Get_Command (const std::string& app_command, const std::vector<std::string>& params) {
    std::string command = "get " + app_command;
    for (const auto& param : params) {
        command += " " + param;
    }

    if (!Send_Command(command)) {
        return GetResponse();
    }

    return Receive_Get_Response();
}

HelpResponse ADCHost::Send_Help_Command (const std::string& app_command) {
    std::string command = "help " + app_command;

    if (!Send_Command(command)) {
        return HelpResponse();
    }

    return Receive_Help_Response();
}
