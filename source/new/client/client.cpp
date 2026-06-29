#include "client.hpp"

#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <pthread.h>
#include <string.h>

Client::Client() {
    Start_IRQ_Thread();
    Start_Data_Thread();
}

Client::~Client() {
    Stop_IRQ_Thread();
    Stop_Data_Thread();
}

void Client::Trigger () {
    m_trigger_queue.enqueue(true);
}

void Client::Continuous_Trigger (bool enable) {
    m_continuous_trigger.store(enable, std::memory_order_release);
}

bool Client::Continuous_Trigger() {
    return m_continuous_trigger.load(std::memory_order_acquire);
}

void Client::Start_IRQ_Thread() {
    m_irq_thread_running.store(true, std::memory_order_release);
    m_irq_thread = std::thread([this]() {
        constexpr int kIrqThreadPriority = 80;
        Configure_Current_Thread_RealTime(SCHED_FIFO, kIrqThreadPriority);

        auto t_start = std::chrono::high_resolution_clock::now();
        auto t_0     = t_start;


        uint32_t int_count        = 0;
        uint32_t v                = 0;
        int32_t  prev_frame_count = 0;
        int32_t  total_errors     = 0;


        std::vector<uint32_t> adc_data;
        uint32_t burst_limit = this->m_adc.Read(ADC_BURST_LIMIT);

        while (this->m_irq_thread_running.load(std::memory_order_acquire)) {

            do {
                v = this->m_adc.Read(MEM_BUF_SEL);
                if (!this->m_irq_thread_running.load(std::memory_order_acquire)) {
                    return;
                }
            } while(this->m_adc.Read(ADC_FRAME2) == 0);
            ++int_count;
            auto t_1 = std::chrono::high_resolution_clock::now();

            // int32_t buffer_select = v & 0x1;
            int32_t frame_count   = static_cast<int32_t>(v >> 1);
            int32_t delta_frames  = frame_count - prev_frame_count;
            prev_frame_count = frame_count;

            adc_data.clear();
            adc_data.reserve(burst_limit);
            for (uint32_t i = 0; i < burst_limit; ++i) {
                adc_data.push_back(this->m_adc.Read_Memory(i));
            }
            this->m_irq_data_queue.enqueue(_ADC_Data{
                .values = adc_data,
                .frame_id = static_cast<uint32_t>(frame_count)
            });

            double timestamp = std::chrono::duration<double, std::milli>(t_1 - t_start).count();
            double duration = std::chrono::duration<double, std::milli>(t_1 - t_0).count();

            if (delta_frames != 1) {
                std::cout << "[APP CLIENT] INFO: observed interval="
                << duration << "ms"
                << " delta_frames=" << delta_frames
                << " timestamp=" << timestamp
                << std::endl;
                ++total_errors;
            }

            if (int_count % (60 * 60) == 0) { // (60 fps * 60 seconds)
                std::cout << "[APP CLIENT] SUMMARY: total_frames=" << frame_count
                    << ", total_interrupts=" << int_count
                    << ", total_errors=" << total_errors
                    << std::endl;
            }

            t_0 = t_1;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));  // Adjust this as needed to balance CPU usage and responsiveness
        }
    });
}

void Client::Stop_IRQ_Thread() {
    m_irq_thread_running.store(false, std::memory_order_release);
    if (m_irq_thread.joinable()) {
        m_irq_thread.join();
    }
}

void Client::Start_Data_Thread() {
    m_data_thread_running.store(true, std::memory_order_release);
    m_data_thread = std::thread([this]() {
        _ADC_Data data;
        bool sent = true;

        AppPacket cpacket {DATA};

        while (this->m_data_thread_running.load(std::memory_order_acquire)) {
            if (this->m_irq_data_queue.try_dequeue(data)) {
                sent = false;
            }

            if (!sent) {
                bool triggered = false;
                if (this->m_trigger_queue.try_dequeue(triggered) || this->m_continuous_trigger.load(std::memory_order_acquire)) {
                    
                    cpacket.Set_Data(data.values);
                    cpacket.Set_FrameID(data.frame_id);
                    auto packet = cpacket.Create();
                    m_send_data_queue.enqueue(packet);
                    sent = true;
                }
            }
        }
    });
}

void Client::Stop_Data_Thread() {
    m_data_thread_running.store(false, std::memory_order_release);
    if (m_data_thread.joinable()) {
        m_data_thread.join();
    }
}

void Client::Set_Consumer(std::function<void(_AppPac&)> callback) {
    m_consumer_callback = std::move(callback);
}

void Client::Start_Consumer_Thread () {
    m_consumer_thread_running.store(true, std::memory_order_release);
    m_consumer_thread = std::thread([this]() {
        _AppPac packet;
        while (this->m_consumer_thread_running.load(std::memory_order_acquire)) {

            if (this->m_send_data_queue.try_dequeue(packet)) {
                if (m_consumer_callback) {
                    m_consumer_callback(packet);
                }
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));  // Sleep briefly to avoid busy waiting
            }

        }
    });
}

void Client::Stop_Consumer_Thread () {
    m_consumer_thread_running.store(false, std::memory_order_release);
    if (m_consumer_thread.joinable()) {
        m_consumer_thread.join();
    }
}


bool Client::Configure_Current_Thread_RealTime(int policy, int priority) {
    int min_priority = sched_get_priority_min(policy);
    int max_priority = sched_get_priority_max(policy);
    if (min_priority == -1 || max_priority == -1) {
        std::cerr << "[CLIENT] WARNING: unable to query scheduler priority range: "
                  << strerror(errno) << std::endl;
        return false;
    }

    int clamped_priority = std::clamp(priority, min_priority, max_priority);
    sched_param params{};
    params.sched_priority = clamped_priority;

    int rc = pthread_setschedparam(pthread_self(), policy, &params);
    if (rc != 0) {
        std::cerr << "[CLIENT] WARNING: failed to set realtime scheduler for irq_thread: "
                  << strerror(rc)
                  << " (run as root or grant CAP_SYS_NICE)" << std::endl;
        return false;
    }

    std::cout << "[CLIENT] INFO: irq_thread scheduler set to SCHED_FIFO priority "
              << clamped_priority << std::endl;
    return true;

}


///////////////////////////////////////////////////////////////
// ADC interface                                             //
///////////////////////////////////////////////////////////////

#define _CLIENT_IMPLT_R(method, type, addr)      \
    type Client::method() {                      \
        return this->m_adc.Read(addr);  \
    }                                            \

#define _CLIENT_IMPLT_W(method, type, addr)                             \
    void Client::method(type value) {                                   \
        this->m_adc.Write(addr, static_cast<uint32_t>(value)); \
    }                                                                   \

#define _CLIENT_IMPLT_WR(method, type, addr)     \
    _CLIENT_IMPLT_W(method, type, addr)          \
    _CLIENT_IMPLT_R(method, type, addr)          \

void Client::adc_RESET () {
    this->m_adc.Write(ADC_RESET, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    this->m_adc.Write(ADC_RESET, 0);
}

void Client::adc_START () {
    this->m_adc.Write(ADC_START, 1);
}

_CLIENT_IMPLT_WR(fpga_HEARTBEAT, uint32_t, HEARTBEAT)
_CLIENT_IMPLT_WR(fpga_ID, uint32_t, ID)
_CLIENT_IMPLT_WR(adc_BURST_LIMIT, uint32_t, ADC_BURST_LIMIT)
_CLIENT_IMPLT_WR(adc_START_PERSIST, bool, ADC_START_PERSIST)
_CLIENT_IMPLT_WR(cd2_PERIOD, uint32_t, CD2_PERIOD)
_CLIENT_IMPLT_WR(cd2_WIDTH, uint32_t, CD2_WIDTH)
_CLIENT_IMPLT_R(adc_TOTAL_PULSES, uint32_t, ADC_TOTAL_PULSES)
_CLIENT_IMPLT_R(adc_FRAME_COUNT, uint32_t, ADC_FRAME_COUNT)
_CLIENT_IMPLT_R(adc_TOTAL_WRITES, uint32_t, ADC_TOTAL_WRITES)
_CLIENT_IMPLT_WR(adc_N_AVERAGE, uint32_t, ADC_N_AVERAGE)
_CLIENT_IMPLT_WR(adc_DELAY, uint32_t, ADC_DELAY)
_CLIENT_IMPLT_WR(adc_ENABLE_MEAN, bool, ADC_ENABLE_MEAN)
_CLIENT_IMPLT_WR(irq_ENABLE, bool, IRQ_ENABLE)
_CLIENT_IMPLT_R(adc_MEM_BUF_SEL, uint32_t, MEM_BUF_SEL)
_CLIENT_IMPLT_WR(adc_MEM_MODES, uint32_t, MEM_MODES)
_CLIENT_IMPLT_W(adc_FRAME_CLEAR, uint32_t, IRQ_FRAME_CLEAR)
_CLIENT_IMPLT_R(adc_FRAME, uint32_t, ADC_FRAME)
_CLIENT_IMPLT_R(adc_FRAME2, uint32_t, ADC_FRAME2)
