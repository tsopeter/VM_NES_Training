#ifndef utils_utils_hpp__
#define utils_utils_hpp__

#include <cstdint>
#include <vector>
#include <torch/torch.h>

namespace Utils {
    uint64_t GetCurrentTime_us ();
    int64_t  GetCurrentTime_s  ();

    void SynchronizeCUDADevices ();

    struct data_structure {
        int64_t iteration;
        double  total_rewards;
    };

    torch::Tensor UpscaleTensor (const torch::Tensor &input, int H, int W);
    torch::Tensor UpscaleTensor (const torch::Tensor &input, int scale);
};


#endif
