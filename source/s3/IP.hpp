#ifndef s3_IP_hpp__
#define s3_IP_hpp__

#include <string>
#include <iostream>
#include <memory>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

struct s3_IP_Packet {
    char *data;         // pointer to the data buffer
    size_t length;      // length of the data buffer
    size_t received;    // number of bytes received
    size_t header_length;   // number of bytes in the header

    char &operator[](size_t);
};

/**
 * @brief  Handles adding headers to the communication overhead
 * 
 */
class s3_Communication_Handler {
public:

    // Field information structure
    struct FieldInfo {
        size_t   length;
        uint32_t id;
        uint64_t magic_number_0 = 0x1234567890ABCDEF;
        uint64_t magic_number_1 = 0xDEADBEEF12345678;
        uint64_t magic_number_2 = 0xBEEFBEEFDEADBEEF;
    };

    s3_Communication_Handler();
    ~s3_Communication_Handler();

    void  CommitData (size_t size);
    std::pair<size_t, char*> AddHeader(char *data, size_t size);

    FieldInfo GetHeader (char *data);

    bool ValidateHeader (FieldInfo &info);

private:
    size_t m_header_size = sizeof(FieldInfo);
    char  *m_commit_data  = nullptr;
    size_t m_commit_size = 0;
    uint32_t m_id = 0;

};

class s3_IP_Client {
public:
    s3_IP_Client(const std::string& ip, uint16_t port);
    ~s3_IP_Client();

    bool connect();
    void disconnect();

    void Transmit(void* data, size_t length);
    int  Receive(void* buffer);
    bool is_connected() const;

    std::string m_ip;
    uint16_t    m_port;
    bool        m_connected;
    int         m_socket_fd = -1;

    s3_Communication_Handler m_handler {};
};

class s3_IP_Host {
public:
    s3_IP_Host(uint16_t port);
    ~s3_IP_Host();

    bool listen_and_accept();
    void disconnect();

    int Receive(void* buffer, size_t length);
    s3_IP_Packet Receive(s3_IP_Packet packet);
    bool is_connected() const;

    uint16_t m_port;
    bool     m_connected;
    int      m_server_fd = -1;
    int      m_client_fd = -1;

    s3_Communication_Handler m_handler {};
};

#endif
