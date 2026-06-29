#include "host.hpp"

Host::Host (std::string ip, uint16_t port) {
    m_host_ip = ip;
    m_port = port;
    m_app_point.Connect(ip, port);
}

Host::~Host () {

}

void Host::Send_Command(const std::string& command) {
    AppPacket packet(CMD);
    packet.Set_Command(command);
    m_app_point.Send(packet);
}

std::vector<_AppPac> Host::Receive() {
    return m_app_point.Receive();
}



