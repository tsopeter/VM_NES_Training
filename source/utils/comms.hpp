#ifndef comms_hpp__
#define comms_hpp__

#include <iostream>
#include <string>
#include <memory>
#include <torch/torch.h>
#include <cstdint>
#include "raylib.h"
#include "../s3/IP.hpp"

enum CommsType : uint8_t {
    COMMS_HOST         = 0,
    COMMS_CLIENT       = 1,
    COMMS_DOUBLE       = 2,
    COMMS_INT          = 3,
    COMMS_INT64        = 4,
    COMMS_IMAGE        = 5,
    COMMS_DISCONNECT   = 6,
    COMMS_STRING       = 7,
    COMMS_DP           = 8,
    COMMS_UNKNOWN_TYPE = 9
};

struct CommsNode {
    CommsType type;
    char *data;

    size_t GetSize();
};

struct CommsDataPacket {
    static const size_t max_size = 10;
    CommsNode nodes[max_size];

    CommsDataPacket();
    
    CommsNode &operator[](size_t index);
    char *CreatePacket(size_t &size);
};



class Comms {
public:
    Comms(CommsType);
    ~Comms();

    void SetParameters (
        int port,
        const std::string ip="" /* Necessary for client, not host */
    );

    void Connect();
    void Transmit(char *data, size_t size);
    void TransmitDouble (double value);
    void TransmitInt (int value);
    void TransmitInt64 (int64_t value);
    void TransmitImage (torch::Tensor);
    void TransmitDataPacket (CommsDataPacket &packet);
    void TransmitDisconnect();
    void TransmitString (const std::string &str);


    CommsType Receive();

    double ReceiveDouble();
    int ReceiveInt();
    int64_t ReceiveInt64();
    std::string ReceiveString();

    torch::Tensor ReceiveImage();
    Texture ReceiveImageAsTexture();
    
    CommsType ReceiveDataPacket ();
    CommsType DP_ReadField ();
    double DP_ReceiveDouble();
    int DP_ReceiveInt();
    int64_t DP_ReceiveInt64();
    Texture DP_ReceiveImageAsTexture();

private:
    s3_IP_Client *client = nullptr;
    s3_IP_Host   *host = nullptr;

    CommsType m_connection_type = COMMS_HOST;

    const size_t m_staging_buffer_size = 1024 * 1024; // 1 MB
    char *m_staging_buffer = nullptr;

    void ResetStagingPacket();

    s3_IP_Packet m_staging_packet {};

    int64_t m_dp_offset = 0; // Offset for data packet reading
};


#endif
