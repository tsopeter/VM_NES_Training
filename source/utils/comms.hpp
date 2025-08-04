#ifndef comms_hpp__
#define comms_hpp__

#include <iostream>
#include <string>
#include <memory>
#include <torch/torch.h>
#include <cstdint>
#include "../s3/IP.hpp"

enum CommsType : uint8_t {
    COMMS_HOST,
    COMMS_CLIENT,
    COMMS_DOUBLE,
    COMMS_INT,
    COMMS_IMAGE
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
    void TransmitDouble (double value);
    void TransmitInt (int value);
    void TransmitImage (const torch::Tensor&);

    CommsType Receive();

    double ReceiveDouble();
    int ReceiveInt();
    torch::Tensor ReceiveImage();

private:
    s3_IP_Client *client = nullptr;
    s3_IP_Host   *host = nullptr;

    CommsType m_connection_type = COMMS_HOST;

    const size_t m_staging_buffer_size = 1024 * 1024; // 1 MB
    char *m_staging_buffer = nullptr;
};


#endif
