#ifndef hcomms_hpp__
#define hcomms_hpp__

#include "../utils/comms.hpp"

enum HCommsModes {
    HCOMMS_HOST = 0,
    HCOMMS_CLIENT = 1
};

struct HCommsDataPacket_Outbound {
    int64_t delta;
    double  reward;
    int64_t step;
    torch::Tensor image; // Expecting a 8-bit grayscale image [H, W]
};

struct HCommsDataPacket_Inbound {
    int64_t delta;
    double  reward;
    int64_t step;
    Texture image; // Expecting a 8-bit grayscale image [H, W]
};

class HComms {
public:
    HComms(std::string ip="", int port=9001);
    HComms(int port);
    ~HComms();

    void Transmit(HCommsDataPacket_Outbound &packet);
    HCommsDataPacket_Inbound Receive();

private:
    HCommsModes mode;
    Comms *comms;
};

#endif // hcomms_hpp__