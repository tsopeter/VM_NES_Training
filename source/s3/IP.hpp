#ifndef s3_IP_hpp__
#define s3_IP_hpp__

#include <string>
#include <iostream>
#include <memory>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

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
};

class s3_IP_Host {
public:
    s3_IP_Host(uint16_t port);
    ~s3_IP_Host();

    bool listen_and_accept();
    void disconnect();

    int Receive(void* buffer);
    bool is_connected() const;

    uint16_t m_port;
    bool     m_connected;
    int      m_server_fd = -1;
    int      m_client_fd = -1;
};

#endif
