#ifndef utils_utils_hpp__
#define utils_utils_hpp__

#include <cstdint>
#include <torch/torch.h>
#include <vector>

namespace Utils {
    uint64_t GetCurrentTime_us ();

    void SynchronizeCUDADevices ();

    struct data_structure {
        int64_t iteration;
        double  total_rewards;
    };
};


#endif
