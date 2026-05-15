#ifndef s5_helpers_hpp__
#define s5_helpers_hpp__

#define CHECKPOINT_DENOTE "#_Checkpoint_Config_File"

#include <torch/torch.h>
#include "raylib.h"

#include "../s2/dataloader.hpp"
#include "../s4/model.hpp"
#include "../s4/optimizer.hpp"
#include "../s4/utils.hpp"
#include "scheduler2.hpp"
#include "distributions.hpp"
#include "../s2/np2lt.hpp"
#include "../s2/plm_device.hpp"

#include "../third-party/concurrentqueue.h"

#include <functional>
#include <iostream>
#include <atomic>

namespace Helpers {

struct Parameters {
    // Windowing
    int monitor_Height = 1600;
    int monitor_Width  = 2716;

    // ADC
    int adc_delay_us = 150.0f;
    int adc_average_n = 10;
    int adc_burst_n   = 20;
    std::string adc_host_ip = "127.0.0.1";
    int adc_host_port = 8000;

    // PLM
    int num_levels = 16;
    PLM_Device_Enum plm_device_enum = PLM_Device_Enum::VISIBLE;

    // Processing Thread
    PDFunction process_fn;

    // total samples = n_samples * burst_n
    int n_samples = 10;
    int burst_n   = 20;

    int n_steps  = 50;
    int n_epochs = 10;

    int upscale_amount = 1;
    int n_iterate = 4;

    int steps = 0;
    double lr = 1e-3;
};

void Setup_Scheduler (
    Parameters   &params,
    Scheduler2   &scheduler,
    s4_Optimizer &optimizer,
    Distributions::Definition &dist_def,
    int Height,
    int Width
);


struct EvalFunctions {
    std::function<torch::Tensor(int)> sample;
    std::function<torch::Tensor(int)> base;
    std::function<void()>             squash;
    std::function<double()>           entropy;
    std::function<double()>           update;
    std::function<double()>           loss;
};

struct Performance {
    double  loss = 0.0f;
    int64_t compute_time_s = 0;
};

namespace Run {
    Performance Evaluate (
        Parameters &,
        Scheduler2 &,
        EvalFunctions &
    );
}

void Iterate (Parameters &, Scheduler2 &);

} // namespace Helpers


#endif
