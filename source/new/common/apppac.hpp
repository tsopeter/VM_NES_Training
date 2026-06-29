#ifndef apppac_hpp__
#define apppac_hpp__

#include <iostream>
#include <string>
#include <vector>

#include "endpoint.hpp"

enum _AppPacType : uint32_t {
    SET  = 1,
    GET  = 2,
    HELP = 3,
    DATA = 4,
    CMD  = 5
};

struct _AppPac {
    _AppPacType type;
    uint32_t    length;
    union {
        uint32_t data[24]; // For get response or data response
        struct {
            bool success; // For set response
            char padding[3]; // Padding to align to 4 bytes
        };
        uint8_t help_message[96]; // For help response (up to 24*4 bytes)
        uint8_t command[96]; // For command packets (up to 24*4 bytes)
    };
    uint32_t    frame_id; // Optional frame ID for correlating requests and responses
};

class AppPacket {
public:
    AppPacket(_AppPacType);
    ~AppPacket();

    void    Set_Success(bool);
    void    Set_Data(const std::vector<uint32_t>&);
    void    Set_Help(const std::string&);
    void    Set_Command(const std::string&);
    void    Set_FrameID (uint32_t);

    _AppPac Create() const;
    _AppPac Interpret(const uint8_t *raw_data, size_t length);

private:
    _AppPacType m_type;
    bool        m_success;  // For set response
    uint32_t    m_length;   // For get response
    uint32_t    m_data[24]; // For get response
    std::string m_help_message; // For help response
    std::string m_command; // For command packets
    uint32_t    m_frame_id; // Optional frame ID for correlating requests and responses
};

class AppPoint {
public:
    AppPoint();
    ~AppPoint();

    void Listen(const std::string& ip, uint16_t port);
    void Connect(const std::string& ip, uint16_t port);

    void Send(const _AppPac  & packet);
    void Send(const AppPacket& packet);
    std::vector<_AppPac> Receive();
private:
    EndPoint m_endpoint;
    std::vector<uint8_t> m_rx_buffer;
};


#endif /* apppac_hpp */