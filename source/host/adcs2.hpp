#ifndef adcs2_hpp
#define adcs2_hpp

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <unistd.h>

#include "adc_host.hpp"
#include "../third-party/concurrentqueue.h"

/**
 * @brief ADCS2 class: High-level interface for controlling ADC acquisition and receiving data
 */
struct ADCS2_CResponse {
    int type; // 0 = set, 1 = get, 2 = help
    bool success; // For set responses
    std::vector<uint32_t> values; // For get responses
    std::string message; // For help responses
};

class ADCS2 {
public:
    ADCS2(std::string host_ip = "192.168.2.1", int port = 8000);
    ~ADCS2();

    void Set_IP_Address(const std::string& ip);
    void Set_Port(int port);

    void set_delay (double delay_us);
    void set_average_n (int n);
    void set_burst_n (int n);

    double get_delay ();
    int    get_average_n ();
    int    get_burst_n ();

    void   reset(); /* Reset ADC hardware */


    void start();
    void close();

    void spin_up_collection();
    void stop_collection();
    void trigger ();
    
    // Response receiving
    bool try_get_data(std::vector<uint32_t>& data);
    bool has_data() const;
    size_t queue_size() const;
    void clear_queue();

    ADCS2_CResponse Receive_Response();
    void        Send_Command (std::string command, std::string app_command, std::vector<int> params);
    bool        Is_Connected() const;

private:
    std::string combine_params (std::string command, std::string app_command, std::vector<int> params);
    std::string clean_commands (std::string command);


    ADCHost m_host;

    std::thread collection_thread;
    std::atomic<bool> collecting{false};
    moodycamel::ConcurrentQueue<std::vector<uint32_t>> adc_data_queue;  // Thread-safe queue for ADC data

    // Command strings
    const std::string trig_cmd = "get adc_start_w";
    const std::string disconnect_cmd = "disconnect";


    int delay     = 0;
    int average_n = 1;
    int burst_n   = 20;

};

#endif // adcs2_hpp