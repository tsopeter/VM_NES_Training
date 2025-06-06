#include "utils.hpp"

// Deterministic random phase mask in [-pi, pi)
torch::Tensor s4_Utils::GetRandomPhaseMask (int64_t h, int64_t w) {
    torch::manual_seed(42);  // Fixed seed for determinism
    return torch::rand({h, w}) * 2 * M_PI - M_PI;
}

Image s4_Utils::TensorToImage (torch::Tensor &t) {
    // Assumes input tensor t is [H, W] or [1, H, W]
    auto t_cpu = t.detach().cpu();
    if (t_cpu.dim() == 3 && t_cpu.size(0) == 1)
        t_cpu = t_cpu.squeeze(0);

    t_cpu = t_cpu.to(torch::kUInt8);

    int64_t h = t_cpu.size(0);
    int64_t w = t_cpu.size(1);

    uint8_t *data = new uint8_t[h*w];
    std::memcpy(data, t_cpu.data_ptr(), h * w);

    Image img {
        .data    = data,
        .width   = static_cast<int>(w),
        .height  = static_cast<int>(h),
        .mipmaps = 1,
        .format  = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
    };

    return img;
}