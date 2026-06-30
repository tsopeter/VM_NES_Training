#ifndef endpoint_hpp__
#define endpoint_hpp__

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdint>
#include "../../third-party/concurrentqueue.h"

class EndPoint {
public:
    EndPoint ();
    ~EndPoint();

    void Listen (const std::string& ip, uint16_t port);
    void Connect(const std::string& ip, uint16_t port);

    void Send(const std::vector<uint8_t>& data);
    std::vector<uint8_t> Receive();
    
    void Close_Sockets();

private:
    void Start_IO_Threads();
    void Stop_IO_Threads();

    std::thread m_send_thread;
    std::atomic<bool> m_send_thread_running{false};
    moodycamel::ConcurrentQueue<std::vector<uint8_t>> m_send_queue;

    std::thread m_receive_thread;
    std::atomic<bool> m_receive_thread_running{false};
    moodycamel::ConcurrentQueue<std::vector<uint8_t>> m_receive_queue;

    std::atomic<bool> m_connected{false};
    int m_server_socket{-1};
    int m_connection_socket{-1};
};

#endif  /* endpoint_hpp__ */
