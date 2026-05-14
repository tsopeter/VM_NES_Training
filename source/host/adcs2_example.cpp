#include "adcs2.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>

std::atomic<bool> running{true};

void signal_handler(int signal) {
    std::cout << "\n[EXAMPLE] Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Parse arguments for test mode
    std::string host_ip = "192.168.2.99";  // Production: listen on specific interface
    int port = 8000;
    
    if (argc > 1 && std::string(argv[1]) == "test") {
        host_ip = "127.0.0.1";  // Test mode: localhost only
        std::cout << "[EXAMPLE] Running in TEST mode (localhost)" << std::endl;
    } else {
        std::cout << "[EXAMPLE] Running in PRODUCTION mode" << std::endl;
    }

    // Create ADCS2 instance
    std::cout << "[EXAMPLE] Creating ADCS2 instance..." << std::endl;
    ADCS2 adc(host_ip, port);

    // Wait for client connection
    std::cout << "[EXAMPLE] Waiting for client connection..." << std::endl;
    while (!adc.Is_Connected() && running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!running) {
        std::cout << "[EXAMPLE] Exiting before connection..." << std::endl;
        return 0;
    }

    std::cout << "[EXAMPLE] Client connected!" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Test 1: Set parameters
    std::cout << "\n=== Test 1: Setting Parameters ===" << std::endl;
    
    std::cout << "[EXAMPLE] Setting delay to 100 us..." << std::endl;
    adc.set_delay(100.0);
    
    std::cout << "[EXAMPLE] Setting average_n to 5..." << std::endl;
    adc.set_average_n(5);
    
    std::cout << "[EXAMPLE] Setting burst_n to 50..." << std::endl;
    adc.set_burst_n(50);

    // Send the parameters to the ADC
    std::cout << "[EXAMPLE] Applying parameters to ADC..." << std::endl;
    adc.start();

    // Test 2: Read parameters back
    std::cout << "\n=== Test 2: Reading Parameters Back ===" << std::endl;
    
    double delay_read = adc.get_delay();
    std::cout << "[EXAMPLE] Delay read back: " << delay_read << " us" << std::endl;
    
    int avg_n_read = adc.get_average_n();
    std::cout << "[EXAMPLE] Average_n read back: " << avg_n_read << std::endl;
    
    int burst_n_read = adc.get_burst_n();
    std::cout << "[EXAMPLE] Burst_n read back: " << burst_n_read << std::endl;

    // Test 3: Trigger acquisition and receive data
    // Only works on production
    if (host_ip != "127.0.0.1") {
        std::cout << "\n=== Test 3: Triggering Acquisition ===" << std::endl;
        std::cout << "[EXAMPLE] Sending trigger command..." << std::endl;
        adc.trigger();

        std::cout << "[EXAMPLE] Waiting for response..." << std::endl;
        ADCS2_CResponse response = adc.Receive_Response();

        if (response.type == 1) {  // GET response
            std::cout << "[EXAMPLE] Received GET response with " << response.values.size() << " values" << std::endl;
            
            if (!response.values.empty()) {
                std::cout << "[EXAMPLE] First 10 values: ";
                for (size_t i = 0; i < std::min(size_t(10), response.values.size()); ++i) {
                    std::cout << response.values[i] << " ";
                }
                std::cout << std::endl;
            }
        } else if (response.type == 0) {  // SET response
            std::cout << "[EXAMPLE] Received SET response: " 
                      << (response.success ? "SUCCESS" : "FAILED") << std::endl;
        } else {
            std::cout << "[EXAMPLE] Received unexpected response type: " << response.type << std::endl;
        }
    } else {
        std::cout << "\n[EXAMPLE] Skipping Test 3 (triggering acquisition) in TEST mode" << std::endl;
    }

    // Test 4: Get some register values
    std::cout << "\n=== Test 4: Reading Individual Registers ===" << std::endl;
    
    std::cout << "[EXAMPLE] Reading heartbeat..." << std::endl;
    adc.Send_Command("get", "heartbeat", {});
    ADCS2_CResponse response;
    response = adc.Receive_Response();
    if (response.type == 1 && !response.values.empty()) {
        std::cout << "[EXAMPLE] Heartbeat: " << response.values[0] << std::endl;
    }

    std::cout << "[EXAMPLE] Reading ID..." << std::endl;
    adc.Send_Command("get", "id", {});
    response = adc.Receive_Response();
    if (response.type == 1 && !response.values.empty()) {
        std::cout << "[EXAMPLE] ID: 0x" << std::hex << response.values[0] << std::dec << std::endl;
    }

    std::cout << "\n[EXAMPLE] All tests complete!" << std::endl;
    std::cout << "[EXAMPLE] Press Ctrl+C to exit..." << std::endl;

    // Keep running until interrupted
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[EXAMPLE] Shutting down..." << std::endl;
    return 0;
}
