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
    COMMS_HOST,
    COMMS_CLIENT,
    COMMS_DOUBLE,
    COMMS_INT,
    COMMS_INT64,
    COMMS_IMAGE,
    COMMS_DISCONNECT,
    COMMS_UNKNOWN_TYPE
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

    CommsType Receive();

    double ReceiveDouble();
    int ReceiveInt();
    int64_t ReceiveInt64();
    torch::Tensor ReceiveImage();
    Texture ReceiveImageAsTexture();

private:
    s3_IP_Client *client = nullptr;
    s3_IP_Host   *host = nullptr;

    CommsType m_connection_type = COMMS_HOST;

    const size_t m_staging_buffer_size = 1024 * 1024; // 1 MB
    char *m_staging_buffer = nullptr;
};


#endif
