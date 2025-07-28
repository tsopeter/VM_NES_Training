#ifndef utils_utils_hpp__
#define utils_utils_hpp__

#include <cstdint>

namespace Utils {
    uint64_t GetCurrentTime_us ();

    void SynchronizeCUDADevices ();

    struct data_structure {
        int64_t iteration;
        double  total_rewards;
    };
};


#endif
