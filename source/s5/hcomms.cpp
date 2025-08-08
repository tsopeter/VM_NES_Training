#include "hcomms.hpp"

HComms::HComms(std::string ip, int port)
: mode(HCOMMS_CLIENT) {
    // This is a client
    mode = HCOMMS_CLIENT;
    std::cout << "INFO: [HComms] Initialized as CLIENT connecting to " << ip << ":" << port << '\n';
    comms = new Comms(CommsType::COMMS_CLIENT);
    comms->SetParameters(port, ip);
    comms->Connect();
}

HComms::HComms(int port)
: mode(HCOMMS_HOST) {
    // This is a host
    mode = HCOMMS_HOST;
    std::cout << "INFO: [HComms] Initialized as HOST on port " << port << '\n';
    comms = new Comms(CommsType::COMMS_HOST);
    comms->SetParameters(port);
    comms->Connect();
}

HComms::~HComms() {
    std::cout << "INFO: [HComms] HComms destroyed.\n";
    delete comms;
}

void HComms::Transmit(HCommsDataPacket_Outbound &packet) {
    if (mode != HCOMMS_CLIENT) {
        throw std::runtime_error("HComms is not a client.");
    }
    std::cout << "INFO: [HComms] Structuring data packet for transmission.\n";

    CommsDataPacket dp;
    dp[0].type = COMMS_INT64;
    dp[0].data = reinterpret_cast<char*>(&packet.step);

    dp[1].type = COMMS_INT64;
    dp[1].data = reinterpret_cast<char*>(&packet.delta);

    dp[2].type = COMMS_DOUBLE;
    dp[2].data = reinterpret_cast<char*>(&packet.reward);

    dp[3].type = COMMS_IMAGE;
    dp[3].data = reinterpret_cast<char*>(&packet.image);

    comms->TransmitDataPacket(dp);
}


HCommsDataPacket_Inbound HComms::Receive() {
    if (mode != HCOMMS_HOST) {
        throw std::runtime_error("HComms is not a host.");
    }

    HCommsDataPacket_Inbound packet;

    CommsType type = comms->Receive();
    if (type != COMMS_DP) {
        throw std::runtime_error("HComms::Receive: Expected COMMS_DP, got different type.");
    }

    // Read fields in order
    type = comms->ReceiveDataPacket();
    if (type != COMMS_INT64) {
        throw std::runtime_error("HComms::Receive: Expected INT64 for step, got different type.");
    }
    packet.step = comms->DP_ReceiveInt64();

    type = comms->DP_ReadField();
    if (type != COMMS_INT64) {
        throw std::runtime_error("HComms::Receive: Expected INT64 for delta, got different type.");
    }
    packet.delta = comms->DP_ReceiveInt64();

    type = comms->DP_ReadField();
    if (type != COMMS_DOUBLE) {
        throw std::runtime_error("HComms::Receive: Expected DOUBLE for reward, got different type.");
    }
    packet.reward = comms->DP_ReceiveDouble();

    type = comms->DP_ReadField();
    if (type != COMMS_IMAGE) {
        throw std::runtime_error("HComms::Receive: Expected IMAGE for image, got different type.");
    }
    packet.image = comms->DP_ReceiveImageAsTexture();

    return packet;
}