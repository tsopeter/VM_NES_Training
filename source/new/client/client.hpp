#ifndef client_hpp__
#define client_hpp__

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>

#include "../adc/adc.hpp"
#include "../common/apppac.hpp"
#include "../common/third-party/concurrentqueue.h"

struct _ADC_Data {
    std::vector<uint32_t> values;
    uint32_t frame_id;
};


class Client {
public:
    Client ();
    ~Client();

    void Trigger();

    void Set_Consumer         (std::function<void(_AppPac&)>);
    void Start_Consumer_Thread();
    void Stop_Consumer_Thread ();

    void Continuous_Trigger(bool enable);
    bool Continuous_Trigger();

    void      fpga_HEARTBEAT (uint32_t value);
    uint32_t  fpga_HEARTBEAT ();

    void      fpga_ID (uint32_t value);
    uint32_t  fpga_ID ();

    void      adc_RESET ();
    
    void      adc_BURST_LIMIT (uint32_t limit);
    uint32_t  adc_BURST_LIMIT ();

    void      adc_START ();

    void      adc_START_PERSIST (bool persist);
    bool      adc_START_PERSIST ();

    void      cd2_PERIOD (uint32_t period);
    uint32_t  cd2_PERIOD ();

    void      cd2_WIDTH (uint32_t width);
    uint32_t  cd2_WIDTH ();

    uint32_t  adc_TOTAL_PULSES ();
    uint32_t  adc_FRAME_COUNT ();
    uint32_t  adc_TOTAL_WRITES ();

    void      adc_N_AVERAGE (uint32_t n);
    uint32_t  adc_N_AVERAGE ();

    void      adc_DELAY (uint32_t delay);
    uint32_t  adc_DELAY ();

    void      adc_ENABLE_MEAN (bool enable);
    bool      adc_ENABLE_MEAN ();

    void      irq_ENABLE (bool enable);
    bool      irq_ENABLE ();

    uint32_t  adc_MEM_BUF_SEL ();

    void      adc_MEM_MODES(uint32_t modes);
    uint32_t  adc_MEM_MODES();

    void     adc_FRAME_CLEAR(uint32_t clear);

    
    uint32_t  adc_FRAME ();
    uint32_t  adc_FRAME2 ();

private:
    ADC m_adc;

    // irq_thread
    void Start_IRQ_Thread ();
    void Stop_IRQ_Thread ();
    std::thread m_irq_thread;
    std::atomic<bool> m_irq_thread_running{false};
    moodycamel::ConcurrentQueue<_ADC_Data> m_irq_data_queue;

    // consumer thread for data processing
    void Start_Data_Thread();
    void Stop_Data_Thread();
    std::thread m_data_thread;
    std::atomic<bool> m_continuous_trigger {false};
    std::atomic<bool> m_data_thread_running{false};
    moodycamel::ConcurrentQueue<bool> m_trigger_queue;
    moodycamel::ConcurrentQueue<_AppPac> m_send_data_queue;

    // consumer callback
    std::function<void(_AppPac&)> m_consumer_callback = nullptr;
    std::thread m_consumer_thread;
    std::atomic<bool> m_consumer_thread_running{false};

    bool Configure_Current_Thread_RealTime (int policy, int priority);
};



#endif