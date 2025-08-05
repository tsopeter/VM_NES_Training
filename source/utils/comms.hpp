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
    COMMS_UNKNOWN_TYPE = 8
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
    void TransmitDisconnect();
    void TransmitString (const std::string &str);

    CommsType Receive();

    double ReceiveDouble();
    int ReceiveInt();
    int64_t ReceiveInt64();
    std::string ReceiveString();

    torch::Tensor ReceiveImage();
    Texture ReceiveImageAsTexture();

private:
    s3_IP_Client *client = nullptr;
    s3_IP_Host   *host = nullptr;

    CommsType m_connection_type = COMMS_HOST;

    const size_t m_staging_buffer_size = 1024 * 1024; // 1 MB
    char *m_staging_buffer = nullptr;

    void ResetStagingPacket();

    s3_IP_Packet m_staging_packet {};
};


#endif
