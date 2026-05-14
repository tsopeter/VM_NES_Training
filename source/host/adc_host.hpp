#ifndef adc_host_hpp
#define adc_host_hpp

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>

struct SetResponse {
    bool success;
};

struct GetResponse {
    std::vector<uint32_t> values;
};

struct HelpResponse {
    std::string message;
};

class ADCHost {
public:
    ADCHost ();
    ~ADCHost ();

    void Set_IP_Address (std::string ip_address);
    void Set_Port (int port);
    void Start_Server ();
    void Stop_Server ();
    
    // Send commands
    bool Send_Command (const std::string& command);
    
    // Receive and decode responses
    SetResponse Receive_Set_Response ();
    GetResponse Receive_Get_Response ();
    HelpResponse Receive_Help_Response ();
    
    // Auto-decode based on response type
    enum class ResponseType {
        SET = 1,
        GET = 2,
        HELP = 3,
        UNKNOWN = 0
    };
    
    ResponseType Get_Response_Type ();
    
    // High-level command helpers
    bool Send_Set_Command (const std::string& app_command, const std::vector<std::string>& params = {});
    GetResponse Send_Get_Command (const std::string& app_command, const std::vector<std::string>& params = {});
    HelpResponse Send_Help_Command (const std::string& app_command);
    
    bool Is_Connected () const { return m_client_connected; }

private:
    void Server_Thread ();
    std::vector<uint8_t> Receive_Raw (size_t buffer_size = 4096);
    uint32_t Decode_U32_Network_Byte_Order (const uint8_t* data);
    
    std::atomic<bool> m_server_running{false};
    std::atomic<bool> m_client_connected{false};
    std::string m_ip_address = "127.0.0.1";
    int m_port              = 8000;
    
    int m_server_socket     = -1;
    int m_client_socket     = -1;
    std::thread m_server_thread;
};

#endif /* adc_host_hpp */