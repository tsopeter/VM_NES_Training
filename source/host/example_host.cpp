#include "adc_host.hpp"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>

// Helper function to convert string to lowercase
std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

int main(int argc, char* argv[]) {
    // Create host instance
    ADCHost host;
    
    // Configure server
    std::string ip;
    int port = 8000;
    
    if (argc > 1 && std::string(argv[1]) == "test") {
        std::cout << "[APP HOST] Running in local test mode..." << std::endl;
        ip = "127.0.0.1";  // Local test mode
    } else if (argc > 1) {
        ip = argv[1];  // Use provided IP address
    } else {
        std::cout << "[APP HOST] No IP address provided, defaulting to 192.168.2.1" << std::endl;
        ip = "192.168.2.1";
    }
    
    host.Set_IP_Address(ip);
    host.Set_Port(port);
    
    // Start server (this will block until client connects)
    host.Start_Server();
    
    // Wait for client connection
    while (!host.Is_Connected()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Example usage:
    // We can send commands to the client and receive responses here for testing purposes
    
    std::cout << "[APP HOST] Ready to send commands to client..." << std::endl;
    
    // Interactive command loop
    while (true) {
        std::cout << ">>> ";
        std::string message;
        std::getline(std::cin, message);
        
        // Convert to lowercase and trim
        message = to_lower(message);
        
        // Remove leading/trailing whitespace
        size_t start = message.find_first_not_of(" \t\r\n");
        size_t end = message.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            message = message.substr(start, end - start + 1);
        }
        
        if (message.empty()) {
            continue;
        }
        
        // Send the command
        if (!host.Send_Command(message)) {
            std::cerr << "[APP HOST] Failed to send command" << std::endl;
            break;
        }
        
        // Check if we should exit
        if (message == "exit" || message == "disconnect") {
            std::cout << "[APP HOST] Exiting..." << std::endl;
            break;
        }
        
        // Receive and decode response
        try {
            auto response_type = host.Get_Response_Type();
            
            if (response_type == ADCHost::ResponseType::SET) {
                auto response = host.Receive_Set_Response();
                std::cout << "!<SET> " << (response.success ? "true" : "false") << std::endl;
            }
            else if (response_type == ADCHost::ResponseType::GET) {
                auto response = host.Receive_Get_Response();
                std::cout << "!<GET> [";
                for (size_t i = 0; i < response.values.size(); ++i) {
                    std::cout << response.values[i];
                    if (i < response.values.size() - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << "]" << std::endl;
            }
            else if (response_type == ADCHost::ResponseType::HELP) {
                auto response = host.Receive_Help_Response();
                std::cout << "!<HELP> " << response.message << std::endl;
            }
            else {
                std::cerr << "[APP HOST] Unknown response type" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[APP HOST] Error receiving response: " << e.what() << std::endl;
        }
    }
    
    // Cleanup
    host.Stop_Server();
    
    return 0;
}
