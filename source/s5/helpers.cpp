#include "helpers.hpp"
#include "../s4/utils.hpp"
#include "../device.hpp"
#include "../utils/utils.hpp"
#include "../s2/np2lt.hpp"
#include <fstream>
#include <filesystem>
#include <future>

Helpers::Performance Helpers::Run::Evaluate (
    Parameters &params,
    Scheduler2 &scheduler,
    EvalFunctions &eval_fn
) {

    int64_t start_time = Utils::GetCurrentTime_s();

    for (int i = 0; i < params.n_samples; ++i) {
        auto action = eval_fn.sample(params.burst_n);

        action = Utils::UpscaleTensor(
            action,
            params.upscale_amount
        );

        scheduler.SetTextureFromTensor (
            action
        );

        Iterate(params, scheduler);
    }
    ++params.steps;

    scheduler.SetVSYNC_Marker();
    scheduler.WaitVSYNC_Diff(2);

    eval_fn.squash();
    double loss = eval_fn.update();

    int64_t end_time = Utils::GetCurrentTime_s ();
    int64_t delta = end_time - start_time;

    Helpers::Performance perf {
        .loss = loss,
        .compute_time_s = delta
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
}