#ifndef utils_utils_hpp__
#define utils_utils_hpp__

#include <cstdint>

namespace Utils {
    uint64_t GetCurrentTime_us ();

    #ifdef __linux__
    void SynchronizeCUDADevices ();
    #endif
};


#endif
