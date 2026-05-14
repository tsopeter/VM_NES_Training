#ifndef scheduler2_hpp__
#define scheduler2_hpp__

// Modules
#include "../host/adcs2.hpp"
#include "../s3/Serial.hpp"
#include "../s4/pencoder.hpp"
#include "../s3/window.hpp"
#include "../s4/optimizer.hpp"
#include "../s2/np2lt.hpp"
#include "../s2/plm_device.hpp"

#include <iostream>
#include <vector>
#include <functional>
#include <thread>

#include <torch/torch.h>

#if defined(__linux__)
    #include "../linux/vsync_timer.hpp"
    #define sched2VSYNCtimer glx_Vsync_timer
#elif defined(__APPLE__)
    #include "../macos/vsync_timer.hpp"
    #define sched2VSYNCtimer macOS_Vsync_Timer
#else
    #error "Unsupported platform"
#endif

class Scheduler2 {
public:
    Scheduler2();
    ~Scheduler2();

    void Start (
        int Monitor = 0,
        int Height  = 1600,
        int Width   = 2716,
        s3_Windowing_Mode wmode=s3_Windowing_Mode::FULLSCREEN,
        s3_TargetFPS_Mode fmode=s3_TargetFPS_Mode::NO_TARGET_FPS,
        int FPS = 30,

        /* FPGA */
        double Delay_us  = 150.0f,
        int    Average_N = 10,
        int    Burst_N   = 20,
        std::string Host_IP = "192.168.2.1",
        int Host_Port = 8000,

        /* PEncoder Properties */
        int PEncoder_Height = 0,
        int PEncoder_Width  = 0,
        int Num_Levels      = 16,
        PLM_Device_Enum plm_device_enum = PLM_Device_Enum::VISIBLE,

        s4_Optimizer *opt = nullptr
    )



private:
    // Windowing
    s3_Window window;


    // FPGA
    ADCS2 adc;
    std::atomic<int64_t> captures_pending {0};
    std::atomic<bool>    enable_fpga {false};

    // VSYNC scheduler
    sched2VSYNCtimer *mvt = nullptr;
    std::function<void(std::atomic<uint64_t>&) > timer_callback;

    void schedule_fpga_capture(std::atomic<uint64_t> &counter);
    std::atomic<uint64_t> m_vsync_count {0};

    // PEncoder
    PEncoder *pen = nullptr; 
    
    // Optimizer
    s4_Optimizer *optimizer = nullptr;


};


#endif
