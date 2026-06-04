#include "scheduler2.hpp"
#include <filesystem>
#include <fstream>

Scheduler2::Scheduler2 () {
   
}

Scheduler2::~Scheduler2 () {
    if (pen) delete pen;
    if (mvt) delete mvt;

    std::cout << "INFO: [Scheduler2::~Scheduler2] Scheduler2 destroyed, resources cleaned up.\n";

}

void Scheduler2::Start (
    int Monitor,
    int Height,
    int Width,
    s3_Windowing_Mode wmode,
    s3_TargetFPS_Mode fmode,
    int FPS,

    double Delay_us,
    int Average_N,
    int Burst_N,
    std::string Host_IP,
    int Host_Port,

    int PEncoder_Height,
    int PEncoder_Width,
    int Num_Levels,
    PLM_Device_Enum plm_device_enum,

    s4_Optimizer *opt,
    PDFunction process_fn
) {
    // Setup Windowing
    window.Height  = Height;
    window.Width   = Width;
    window.monitor = Monitor;
    window.wmode   = wmode;
    window.fmode   = fmode;
    window.fps     = FPS;

    // Setup FPGA
    adc.set_delay(Delay_us);
    adc.set_average_n(Average_N);
    adc.set_burst_n(Burst_N);
    burst_n = Burst_N;

    // Setup Optimizer
    optimizer = opt;


    // Start up window
    window.load();
    m_alpha_ignore_shader = LoadShader(nullptr, "source/shaders/alpha_ignore.fs");
    
    // Start up FPGA
    adc.Set_IP_Address(Host_IP);
    adc.Set_Port(Host_Port);
    adc.start();
    adc.spin_up_collection();


    // Setup PEncoder
    pen = new PEncoder(
        0,
        0,
        PEncoder_Height, 
        PEncoder_Width, 
        Num_Levels,
        plm_device_enum
    );
    pen->init_pbo();
    this->plm_device_enum = plm_device_enum;
    
    // Setup VSYNC timer
    timer_callback = [this](std::atomic<uint64_t> &arg) {
        this->schedule_fpga_capture(arg);
    };
    mvt = new sched2VSYNCtimer(0, timer_callback);

    // Start capture thread
    StartCaptureThread();


    if (process_fn == nullptr) {
        std::cerr << "ERROR: [Scheduler2::Start] process_fn is nullptr. Please provide a valid processing function.\n";
        throw std::runtime_error("process_fn is nullptr");
    }
    // Setup Processing Thread
    StartProcessThread(process_fn);
}

//////////////////////////////////////////////////////////////////
// VSYNC
void Scheduler2::schedule_fpga_capture(std::atomic<uint64_t> &counter) {
    m_vsync_count.store(
        counter.load(std::memory_order_acquire),
        std::memory_order_release
    );

    if (!enable_fpga.load(std::memory_order_acquire)) {
        return;
    }

    captures_pending.fetch_add(1, std::memory_order_release);
    enable_fpga.store(false, std::memory_order_release);

    adc.trigger();
    std::cout << "INFO: [Scheduler2::schedule_fpga_capture] VSYNC captured, FPGA triggered.\n";
}

//////////////////////////////////////////////////////////////////
// Capture Thread to read
// from ADC
void Scheduler2::StartCaptureThread () {
    capture_thread_running.store(true, std::memory_order_release);
    capture_thread = std::thread([this]() {
        while (capture_thread_running.load(std::memory_order_acquire)) {
            if (captures_pending.load(std::memory_order_acquire) <= 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                continue;
            }

            // Read from ADC
            std::vector<uint32_t> data;
            while (!adc.try_get_data(data)) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }

            std::cout << "INFO: [Scheduler2::CaptureThread] Received ADC data, processing...\n";

            // To tensor
            torch::Tensor tensor_data = torch::from_blob(
                data.data(),
                {static_cast<long>(data.size())},
                torch::kUInt32
            ).clone(); // Clone to own the memory

            // Queue to Processing Pipeline
            process_queue.enqueue(tensor_data);

            captures_pending.fetch_sub(1, std::memory_order_release);
        }
    });
}

void Scheduler2::set_device (const torch::Device &device) {
    this->device = device;
}

void Scheduler2::StartProcessThread (PDFunction process_function) {
    processing_thread_running.store(true, std::memory_order_release);
    processing_thread = std::thread([this, process_function]() {
        while (processing_thread_running.load(std::memory_order_acquire)) {
            torch::Tensor data;
            if (!process_queue.try_dequeue(data)) {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                continue;
            }

            // data -> CaptureData
            CaptureData capture_data{
                .data = data
            };

            // Process with user function
            torch::Tensor result = process_function(capture_data);

            // Place to result queue
            result_queue.enqueue(result);
            results_count.fetch_add(1, std::memory_order_release);
        }
    });
}

double Scheduler2::Update() {
    std::cout << "INFO: [Scheduler2::Update] Starting update cycle, waiting for results from processing thread...\n";
    // Dequeue results from results queue
    uint64_t number_of_rewards = frame_count;
    std::vector<torch::Tensor> results;

    std::cout << "INFO: [Scheduler2::Update] Expecting " << number_of_rewards << " rewards to process.\n";

    for (int i = 0; i < number_of_rewards; ++i) {
        torch::Tensor reward;
        while (!result_queue.try_dequeue(reward)) {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
        results.push_back(reward);
        std::cout << "INFO: [Scheduler2::Update] Received reward " << i + 1 << "/" << number_of_rewards << " from processing thread.\n";
    }

    // The rewards are structed as
    /*
        [
            [r0, r1, r2, ... r19],
            [r20, r21, r22, ... r39],
            ...
            [r(n-20), r(n-19), r(n-18), ... r(n-1)]
        ]
    */
    // We want to place them in a 1D tensor of shape [n]
    torch::Tensor rewards_tensor = torch::cat(results).to(device);

    // Average rewards
    double average_reward = rewards_tensor.mean().item<double>();

    // Optimize
    optimizer->step(rewards_tensor);

    frame_count = 0; // Reset frame count after processing
    return average_reward;
}

void Scheduler2::ReadFromADC () {
    std::cout << "INFO: [Scheduler2::ReadFromADC] Waiting for FPGA capture to complete...\n";
    /*
    while (enable_fpga.load(std::memory_order_acquire));           // wait till the current capture is done
    enable_fpga.store(true, std::memory_order_release);            // signal to start capture
    while (captures_pending.load(std::memory_order_acquire) != 0); // wait till capture is done
    std::cout << "INFO: [Scheduler2::ReadFromADC] FPGA capture complete, data should be in the queue.\n";
    */
    while (!adc.recv_valid()) {}
    adc.trigger();
    captures_pending.fetch_add(1, std::memory_order_release);
    ++frame_count; // Increment frame count after capture is done
    std::cout << "INFO: [Scheduler2::ReadFromADC] Frame count incremented to " << frame_count << "\n";
}

void Scheduler2::DrawTextureToScreen () {
    int centerX = (window.Width - m_texture.width) / 2;
    int centerY = (window.Height - m_texture.height) / 2;

    int offsetX = (plm_device_enum == PLM_Device_Enum::VISIBLE) ? 0 : 2;

    BeginDrawing();
    BeginShaderMode(m_alpha_ignore_shader);
    ClearBackground(BLACK);

    DrawTexturePro(
        m_texture,
        {0.0f, 0.0f, static_cast<float>(m_texture.width), static_cast<float>(m_texture.height)},
        {static_cast<float>(centerX + offsetX), static_cast<float>(centerY), static_cast<float>(m_texture.width), static_cast<float>(m_texture.height)},
        {0.0f, 0.0f},
        0.0f,
        WHITE
    );
    
    EndShaderMode();
    EndDrawing();
}

uint64_t Scheduler2::GetVSYNC_Count () {
    return m_vsync_count.load(std::memory_order_acquire);
}

void Scheduler2::SetVSYNC_Marker () {
    m_vsync_marker = GetVSYNC_Count();
}

void Scheduler2::WaitVSYNC_Diff (uint64_t target_diff) {
    while ((GetVSYNC_Count() - m_vsync_marker) < target_diff) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

void Scheduler2::StopWindow () {
    // Do nothing
}

void Scheduler2::StopFPGA () {
    adc.close ();
}

void Scheduler2::StopThreads () {
    // Stop processing thread
    processing_thread_running.store(false, std::memory_order_release);
    if (processing_thread.joinable()) {
        processing_thread.join();
    }

    // End capture thread
    capture_thread_running.store(false, std::memory_order_release);
    if (capture_thread.joinable()) {
        capture_thread.join();
    }
}

void Scheduler2::SetTextureFromTensor (const torch::Tensor &tensor) {
    torch::Tensor timage;
    if (m_categorical_mode) {
        timage = pen->MEncode_u8Tensor_Categorical(tensor).contiguous().to(torch::kInt32); // Categorical
    } else {
        timage = pen->MEncode_u8Tensor5(tensor).contiguous().to(torch::kInt32); // Normal
    }
    
    if (m_texture.width > 0 && m_texture.height > 0) {
        printf("Texture is valid!\n");
        UnloadTexture(m_texture);
    } else {
        printf("Texture not loaded.\n");
    }
    

    m_texture = pen->u8Tensor_Texture_CPU(timage);
    std::cout << "INFO: [Scheduler2::SetTextureFromTensor] Texture size: " << m_texture.width << "x" << m_texture.height << '\n';
}

void Scheduler2::UnloadTextures () {
    if (m_texture.width > 0 && m_texture.height > 0) {
        UnloadTexture(m_texture);
        m_texture.width = 0;
        m_texture.height = 0;
        std::cout << "INFO: [Scheduler2::UnloadTexture] Texture unloaded.\n";
    }
}