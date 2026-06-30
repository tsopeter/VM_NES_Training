#include "helpers.hpp"
#include "../s4/utils.hpp"
#include "../device.hpp"
#include "../utils/utils.hpp"
#include "../s2/np2lt.hpp"
#include <fstream>
#include <filesystem>
#include <future>

#define RED_TEXT_START "\033[31m"
#define RED_TEXT_END   "\033[0m"
#define GREEN_TEXT_START "\033[32m"
#define GREEN_TEXT_END   "\033[0m"
#define BLUE_TEXT_START "\033[34m"
#define BLUE_TEXT_END   "\033[0m"
#define YELLOW_TEXT_START "\033[33m"
#define YELLOW_TEXT_END   "\033[0m"
#define MAGENTA_TEXT_START "\033[35m"
#define MAGENTA_TEXT_END   "\033[0m"
#define CYAN_TEXT_START "\033[36m"
#define CYAN_TEXT_END   "\033[0m"

Helpers::Performance Helpers::Run::Evaluate (
    Parameters &params,
    Scheduler2 &scheduler,
    EvalFunctions &eval_fn
) {

    int64_t start_time = Utils::GetCurrentTime_us ();

    using Clock = std::chrono::steady_clock;


    for (int i = 0; i < params.n_samples; ++i) {

        auto start = Clock::now();
        auto action = eval_fn.sample(params.burst_n);
        action = Utils::UpscaleTensor(
            action,
            params.upscale_amount
        );

        scheduler.SetTextureFromTensor (
            action
        );
        auto end = Clock::now();

        // get in ms
        const double duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "INFO: [Helpers::Run::Evaluate] Sampled and set texture in " << duration_ms << " ms\n";

        Iterate(params, scheduler);
        ++params.steps;
    }

    scheduler.SetVSYNC_Marker();
    scheduler.WaitVSYNC_Diff(2);

    eval_fn.squash();
    std::cout << "INFO: [Helpers::Run::Evaluate] Step Count: " << params.steps << "\n";
    double loss = eval_fn.update();

    int64_t end_time = Utils::GetCurrentTime_us ();
    int64_t delta    = end_time - start_time;
    auto    entropy  = eval_fn.entropy();

    std::cout << RED_TEXT_START "INFO: [Helpers::Run::Evaluate] elapsed_time= " << 
        delta / 1e3 << " ms loss= " << loss << " entropy= " << entropy << RED_TEXT_END "\n";

    Helpers::Performance perf {
        .loss = loss,
        .compute_time_s = delta / 1e6,
        .entropy = entropy
    };

    return perf;
}

void Helpers::Iterate (Parameters &params, Scheduler2 &scheduler) {
    for (int i = 0; i < params.n_iterate; ++i) {
        scheduler.DrawTextureToScreen ();

        scheduler.SetVSYNC_Marker ();
        scheduler.WaitVSYNC_Diff (1);
    }
    scheduler.ReadFromADC ();
}

Helpers::Parameters::Parameters () {
    monitor_Height = 1600;
    monitor_Width  = 2716;

    adc_delay_us  = 150.0f;
    adc_average_n = 10;
    adc_burst_n   = 20;
    adc_host_ip   = "127.0.0.1";
    adc_host_port = 8000;

    num_levels = 16;
    plm_device_enum = PLM_Device_Enum::VISIBLE;

    n_samples = 10;
    burst_n   = 20;
    n_steps  = 50;
    n_epochs = 10;
    upscale_amount = 1;
    n_iterate = 4;
    lr = 1e-3;
    steps = 0;


    process_fn = [this](CaptureData data)->torch::Tensor {
        // Capture data is stored as
        // Convert to CUDA
        auto data_cuda = data.data.to(DEVICE);

        // Convert to float
        data_cuda = data_cuda.to(torch::kFloat32);

        // It is originally stored as 32-bit integers (actually 16-bit offset binary)
        // We need to convert it to a range of [-1, 1]
        data_cuda = (data_cuda - 32768.0f) / 32768.0f;
        data_cuda *= 4.0f;

        // do not invert if adc_invert is false, otherwise invert the signal
        if (!this->adc_invert)
            data_cuda = -data_cuda;

        std::cout << "INFO: [Helpers::Parameters::process_fn] Processed ADC data...\n";

        return data_cuda;
    };



}

void Helpers::Setup_Scheduler (
    Parameters &params,
    Scheduler2 &scheduler,
    s4_Optimizer &optimizer,
    Distributions::Definition &dist_def,
    int Height,
    int Width
) {
    scheduler.Start(
        0, // Monitor
        params.monitor_Height, // Height
        params.monitor_Width,  // Width
        s3_Windowing_Mode::FULLSCREEN,
        s3_TargetFPS_Mode::NO_TARGET_FPS,
        30, // FPS

        params.adc_delay_us, // Delay_us
        params.adc_average_n, // Average_N
        params.adc_burst_n,   // Burst_N
        params.adc_host_ip,   // Host_IP
        params.adc_host_port, // Host_Port

        Height, // PEncoder_Height
        Width,  // PEncoder_Width
        params.num_levels, // Num_Levels
        params.plm_device_enum, // plm_device_enum

        &optimizer, // opt,

        params.process_fn // process_fn
    );

    scheduler.set_device(DEVICE);
    if (dist_def.get_name() == "categorical")
        scheduler.m_categorical_mode = true;
    else
        scheduler.m_categorical_mode = false;

    optimizer.entropy_regularization = params.entropy_regularization;
    optimizer.entropy_coeff = params.entropy_coeff;
}