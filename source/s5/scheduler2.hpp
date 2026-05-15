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

struct CaptureData {
    torch::Tensor data;
};

using PDFunction = std::function<torch::Tensor(CaptureData)>;

class Scheduler2 {
public:
    Scheduler2();
    ~Scheduler2();

    void StopWindow(); // Does nothing
    void StopFPGA();
    void UnloadTextures();

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

        s4_Optimizer *opt = nullptr,

        PDFunction process_fn = nullptr
    );

    void set_device (const torch::Device &device);

    ///////////////////////////////////////
    // Threads
    void schedule_fpga_capture(std::atomic<uint64_t> &counter);
    void StartCaptureThread ();
    void StartProcessThread (PDFunction process_function);
    void StopThreads ();

    ///////////////////////////////////////
    // FPGA Reading
    void ReadFromADC ();

    ///////////////////////////////////////
    // Update
    double Update();

    void DrawTextureToScreen ();
    void SetTextureFromTensor (const torch::Tensor &tensor);
    bool m_categorical_mode = false;

    void SetVSYNC_Marker ();
    void WaitVSYNC_Diff (uint64_t target_diff);

private:
    // Windowing
    s3_Window window;
    Texture m_texture;

    // Data Pipeline
    std::atomic<bool> processing_thread_running {false};
    std::thread processing_thread;
    moodycamel::ConcurrentQueue<torch::Tensor> process_queue;  // Thread-safe queue for ADC data
    moodycamel::ConcurrentQueue<torch::Tensor> result_queue;   // Thread-safe queue for results


    // FPGA
    ADCS2 adc;
    std::atomic<int64_t> captures_pending {0};
    std::atomic<bool>    enable_fpga {false};

    std::thread capture_thread;
    std::atomic<bool> capture_thread_running {false};
    std::atomic<bool> enable_capture {false};

    // VSYNC scheduler
    sched2VSYNCtimer *mvt = nullptr;
    uint64_t m_vsync_marker = 0;
    std::function<void(std::atomic<uint64_t>&) > timer_callback;
    std::atomic<uint64_t> m_vsync_count {0};

    uint64_t GetVSYNC_Count ();

    // PEncoder
    PEncoder *pen = nullptr; 
    
    // Optimizer
    s4_Optimizer *optimizer = nullptr;

    // device
    torch::Device device = DEVICE;
    PLM_Device_Enum plm_device_enum = PLM_Device_Enum::VISIBLE;

    ///////////////////////////////////////////////////
    // Iterators
    uint64_t frame_count = 0;
    uint64_t burst_n     = 20;
};


#endif
