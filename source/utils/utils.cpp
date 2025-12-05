#include "utils.hpp"
#include <chrono>

uint64_t Utils::GetCurrentTime_us () {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

int64_t Utils::GetCurrentTime_s () {
    return std::chrono::duration_cast<std::chrono::seconds>(
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
#else
#include <iostream>

void Utils::SynchronizeCUDADevices () {
    std::cerr << "INFO: [Utils::SynchronizeCUDADevices] Not available on macOS or Windows.\n";
}
#endif

torch::Tensor Utils::UpscaleTensor (const torch::Tensor &input, int H, int W) {
    // Input can be [H, W], [C, H, W] or [B, C, H, W]
    torch::NoGradGuard no_grad;

    // First make sure input is 4D
    torch::Tensor x;
    if (input.dim() == 2) {
        x = input.unsqueeze(0).unsqueeze(0); // [1, 1, H, W]
    }
    else if (input.dim() == 3) {
        x = input.unsqueeze(0); // [1, C, H, W]
    }

    // Upscale
    x = torch::nn::functional::interpolate(
        x,
        torch::nn::functional::InterpolateFuncOptions()
            .size(std::vector<int64_t>({H, W}))
            .mode(torch::kNearest)
            //.align_corners(false)
    );

    // Return to original shape
    if (input.dim() == 2) {
        x = x.squeeze(0).squeeze(0); // [H, W]
    } else if (input.dim() == 3) {
        x = x.squeeze(0); // [C, H, W]
    }

    return x;
}

torch::Tensor Utils::UpscaleTensor (const torch::Tensor &input, int scale) {
    if (scale <= 0) {
        throw std::runtime_error("Utils::UpscaleTensor: scale must be positive integer.\n");
    }

    if (scale == 1) {
        return input;
    }

    // Input can be [H, W], [C, H, W] or [B, C, H, W]
    
    torch::Tensor x;
    if (input.dim() == 2) {
        x = input.unsqueeze(0).unsqueeze(0); // [1, 1, H, W]
    }
    else if (input.dim() == 3) {
        x = input.unsqueeze(0); // [1, C, H, W]
    } else {
        x = input;
    }

    // use repeat_interleave to upscale by interger
    x = x.repeat_interleave(scale, -2).repeat_interleave(scale, -1);

    // Return to original shape
    if (input.dim() == 2) {
        x = x.squeeze(0).squeeze(0); // [H, W]
    } else if (input.dim() == 3) {
        x = x.squeeze(0); // [C, H, W]
    }

    return x;
}