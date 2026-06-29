#ifndef host_hpp__
#define host_hpp__

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>

#include "common/apppac.hpp"
#include "common/third-party/concurrentqueue.h"

class Host {
public:
    Host  (std::string ip, uint16_t port);
    ~Host ();

    void Send_Command(const std::string& command);
    std::vector<_AppPac> Receive();

private:
    AppPoint m_app_point;
    std::string m_host_ip = "192.168.2.10";
    uint16_t m_port = 8000;
};


#endif /* host_hpp__ */