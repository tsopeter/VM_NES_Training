#include "utils.hpp"
#include <chrono>

uint64_t Utils::GetCurrentTime_us () {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

#ifdef __linux__
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

void Utils::SynchronizeCUDADevices () {
    cudaDeviceSynchronize();
}
#endif
