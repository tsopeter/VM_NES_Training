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

struct CaptureData {
    torch::Tensor image;
    torch::Tensor full;
    int           label;
    int           batch_id;
    int           action_id;
};

using PDFunction = std::function<std::pair<torch::Tensor, bool>(CaptureData)>;

struct Scheduler2_CheckPoint {
    int    batch_id;
    double training_accuracy;
    double validation_accuracy;
    torch::Tensor phase;
    double kappa;

    int64_t step;
    double reward;


    std::string dataset_path;

    std::string checkpoint_dir;
    std::string checkpoint_name; /* optional */
};

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
        int Height=480, 
        int Width=640, 
        float ExposureTime=59.0f, 
        int BinningHorizontal=1,
        int BinningVertical=1,
        int cam_LineTrigger=3,
        bool cam_UseZones=false,
        int cam_NumberOfZones=4,
        int cam_ZoneSize=60,
        bool cam_use_centering=true,
        int cam_offset_x=0,
        int cam_offset_y=0
    );

    void SetupPEncoder (
        int pencoder_Height=1600,
        int pencoder_Width=2560
    );

    void StartWindow();
    void StopWindow();

    void StartCamera();
    void StopCamera();

    void SetTextureFromTensor(const torch::Tensor &tensor);
    void SetTextureFromTensorTiled (const torch::Tensor &tensor);
    void Training_SaveMaskToDrive(const std::string &);

    void DrawTextureToScreen ();
    void DrawTextureToScreenTiled ();
    void DrawTextureToScreenCentered ();

    void Validation_SetTileParams(int);
    void Validation_SetDatasetTexture(Texture);
    void Validation_SetMask(const torch::Tensor&);
    void Validation_DrawToScreen();
    void Validation_SaveMaskToDrive(const std::string&);

    void SetOptimizer(s4_Optimizer *opt);

    double Update ();
    void   Dump ();
    void   Dump (int);

    void ReadFromCamera();

    // Pipeline for processing the data
    /**
     * @brief Processes the data pipeline.
     * 
     * Important notice:
     * This method will run on a separate thread and will continuously
     * process data from the camera2process queue using the provided
     * process_function. The processed data will be stored in the outputs queue.
     * 
     * Note: The input tensor will differ based on how the camera is configured.
     * If the zones are disabled, the input tensor will be a single channel image
     * with shape [Height, Width].
     * 
     * However, if the zones are enabled, the input tensor will be
     * a multi-channel image with shape [NumberOfZones * NumberOfZones, ZoneSize, ZoneSize].
     */
    void ProcessDataPipeline(
        PDFunction process_function
    );

    // Setup the VSYNC timing channels
    void SetupVSYNCTimer();

    // Setup the capture system
    void StartCameraThread();

    torch::Tensor GetSampleImage ();
    void SaveSampleImage(const std::string &filename);
    void DisposeSampleImages();

    // Enables or Disables sample image capture.
    // This frees up some time, but not much...
    void EnableSampleImageCapture();
    void DisableSampleImageCapture();

    void StopThreads();
    void SetRewardDevice(const torch::Device &device);

    void SetSubTextures(Texture, int);
    void EnableSubTexture(int);
    void DisableSubTexture(int);
    void DrawSubTexturesOnly();

    void SetLabel(int);
    void SetLabel(std::vector<int>);
    void SetBatch_Id(int);
    void SetAction_Id(int);
    void SetBatchSize(int);


    void SetVSYNC_Marker ();
    void WaitVSYNC_Diff  (uint64_t diff=1);

    void Start (
        /* Windowing */
        int monitor=0,
        int Height=1600, 
        int Width=2560, 
        s3_Windowing_Mode wmode=s3_Windowing_Mode::FULLSCREEN,
        s3_TargetFPS_Mode fmode=s3_TargetFPS_Mode::NO_TARGET_FPS,
        int fps=30,

        /* Camera */
        int cam_Height=480, 
        int cam_Width=640, 
        float cam_ExposureTime=59.0f, 
        int cam_BinningHorizontal=1,
        int cam_BinningVertical=1,
        int cam_LineTrigger=3,
        bool cam_UseZones=false,
        int cam_NumberOfZones=4,
        int cam_ZoneSize=60,
        bool cam_use_centering=true,
        int cam_offset_x=0,
        int cam_offset_y=0,

        /* PEncoder properties */
        int pencoder_Height=0,
        int pencoder_Width=0,

        /* Optimizer */
        s4_Optimizer *opt=nullptr,

        /* Processing function */
        PDFunction process_function=nullptr
    );

    void SaveCheckpoint(Scheduler2_CheckPoint);
    Scheduler2_CheckPoint LoadCheckpoint(const std::string&);

private:
    // Private methods 
    std::pair<torch::Tensor, torch::Tensor> ReadCamera();
    void CameraThread();
    std::pair<torch::Tensor, torch::Tensor> ReadCamera_1();
    std::pair<torch::Tensor, torch::Tensor> ReadCamera_2();
    std::pair<torch::Tensor, torch::Tensor> ReadCamera_3();

    torch::Tensor GetSampleImage_1();
    torch::Tensor GetSampleImage_2();

    // Handles the timing between VSYNC pulses and camera
    sched2VSYNCtimer *mvt = nullptr;
    std::function<void(std::atomic<uint64_t>&)> timer_callback; 
    Serial serial;
    void schedule_camera_capture(std::atomic<uint64_t>& timestamp);
    void wait_till_capture_pending_is_zero ();
    void wait_till_capture_enable_is_false ();
    void send_ready_to_capture ();

    Texture m_texture;
    Texture m_sub_textures[10];
    bool m_sub_textures_enable[10];

    Texture m_val_texture;
    Texture m_val_mask;
    int     m_val_tile_size;

    void DrawSubTexturesToScreen();
    void DrawSubTexturesToScreenCentered();

    torch::Tensor Uninterleave(torch::Tensor&);

    //
    // Sample image for viewing purposes
    moodycamel::ConcurrentQueue<torch::Tensor> sample_images;

    //
    // Sample image control
    std::atomic<bool> sample_image_capture_enabled {false};


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
    Shader sub_shader;
    Shader val_shader;

    //
    // Optimizer
    s4_Optimizer *opt = nullptr;

    //
    // Data buffers
    moodycamel::ConcurrentQueue<std::vector<torch::Tensor>> camera2process;
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

    //
    // Set reward device
    torch::Device reward_device = torch::kCPU;

    //
    //
    std::atomic<int> m_label;
    std::vector<int> m_labels;
    int m_batch_id = 0;
    int m_action_id = 0;
    int m_batch_size = 0;

    void BindShader (Shader&, float,float,Texture,Texture);

    uint64_t GetVSYNC_count ();
    std::atomic<uint64_t> m_vsync_count {0};
    
    uint64_t m_vsync_marker;

};


#endif
