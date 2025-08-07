#ifndef scheduler2_hpp__
#define scheduler2_hpp__

// Modules
#include "cam2.hpp"
#include "../s3/Serial.hpp"
#include "../s4/pencoder.hpp"
#include "../s3/window.hpp"
#include "../s4/optimizer.hpp"

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
    //
    //  Constants
    //
    static constexpr int maximum_number_of_frames_in_image = 20;
    static constexpr char port_name[] = "/dev/ttyACM0";
    static constexpr int baud_rate = 115200;

    //
    // Public objects
    Cam2 camera; // Camera object
    s3_Window window; // Window object

    Scheduler2();
    ~Scheduler2();

    void SetupWindow(
        int monitor=0,
        int Height=1600, 
        int Width=2560, 
        s3_Windowing_Mode wmode=s3_Windowing_Mode::FULLSCREEN,
        s3_TargetFPS_Mode fmode=s3_TargetFPS_Mode::NO_TARGET_FPS,
        int fps=30
    );

    void SetupCamera (
        int Height=240, 
        int Width=320, 
        float ExposureTime=59.0f, 
        int BinningHorizontal=2,
        int BinningVertical=2
    );

    void SetupPEncoder ();

    void StartWindow();
    void StopWindow();

    void StartCamera();
    void StopCamera();

    void SetTextureFromTensor(const torch::Tensor &tensor);

    void DrawTextureToScreen ();

    void SetOptimizer(s4_Optimizer *opt);

    double Update ();

    void ReadFromCamera();

    // Pipeline for processing the data
    void ProcessDataPipeline(
        std::function<torch::Tensor(torch::Tensor)> process_function
    );

    // Setup the VSYNC timing channels
    void SetupVSYNCTimer();

    // Setup the capture system
    void StartCameraThread();

    torch::Tensor GetSampleImage ();
    void SaveSampleImage(const std::string &filename);
    void DisposeSampleImages();

    void StopThreads();

    void Start (
        /* Windowing */
        int monitor=0,
        int Height=1600, 
        int Width=2560, 
        s3_Windowing_Mode wmode=s3_Windowing_Mode::FULLSCREEN,
        s3_TargetFPS_Mode fmode=s3_TargetFPS_Mode::NO_TARGET_FPS,
        int fps=30,

        /* Camera */
        int cam_Height=240, 
        int cam_Width=320, 
        float cam_ExposureTime=59.0f, 
        int cam_BinningHorizontal=2,
        int cam_BinningVertical=2,

        /* Optimizer */
        s4_Optimizer *opt=nullptr,

        /* Processing function */
        std::function<torch::Tensor(torch::Tensor)> process_function=nullptr
    );

private:
    // Private methods 
    torch::Tensor ReadCamera();
    void CameraThread();

    // Handles the timing between VSYNC pulses and camera
    sched2VSYNCtimer *mvt = nullptr;
    std::function<void(std::atomic<uint64_t>&)> timer_callback; 
    Serial serial;
    void schedule_camera_capture(std::atomic<uint64_t>& timestamp);
    void wait_till_capture_pending_is_zero ();
    void wait_till_capture_enable_is_false ();
    void send_ready_to_capture ();

    Texture m_texture;

    //
    // Sample image for viewing purposes
    moodycamel::ConcurrentQueue<torch::Tensor> sample_images;


    //
    // Camera Tracking
    std::atomic<bool>    enable_capture {false};
    std::atomic<int64_t> captures_pending {0};


    //
    // These are used to create the texture from a tensor
    // using PENcoder.
    PEncoder *pen = nullptr;

    //
    // Shader is used for ignoring alpha channel
    Shader shader;

    //
    // Optimizer
    s4_Optimizer *opt = nullptr;

    //
    // Data buffers
    moodycamel::ConcurrentQueue<torch::Tensor> camera2process;
    moodycamel::ConcurrentQueue<torch::Tensor> outputs;

    //
    // Threads
    std::thread processing_thread;
    std::thread camera_thread;

    //
    // Thread control
    std::atomic<bool> end_processing {false};
    std::atomic<bool> end_camera {false};


    //
    // Update control
    int64_t number_of_frames_sent = 0;
};


#endif
