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

torch::Tensor s4_Utils::GSAlgorithm(const torch::Tensor &target, int iterations) {
    using namespace torch::indexing;

    // Ensure target is float32
    torch::Tensor Target = target.to(torch::kFloat32);

    int Height = Target.size(0);
    int Width  = Target.size(1);

    // Define phase-normalization function
    auto phase = [](const torch::Tensor& p) {
        return p / (p.abs() + 1e-8);
    };

    // Initialize with random phase
    auto rand_phase = torch::rand({Height, Width}, torch::kFloat32) * 2 * M_PI;
    rand_phase = rand_phase.to(Target.device());
    auto Object = torch::polar(torch::ones_like(rand_phase), rand_phase); // magnitude 1, phase = rand_phase

    for (int i = 0; i < iterations; ++i) {
        auto U  = torch::fft::ifft2(Object);
        auto Up = Target * phase(U);
        auto D  = torch::fft::fft2(Up);
        Object  = phase(D);
    }

    std::cout << "INFO: [GSAlgorithm] Completed " << iterations << " iterations of Gerchberg-Saxton algorithm.\n";
    return torch::angle(Object);
}

torch::Tensor s4_Utils::GSAlgorithm(const torch::Tensor &target, const torch::Tensor &initial, int iterations) {
    using namespace torch::indexing;

    // Ensure target is float32
    torch::Tensor Target = target.to(torch::kFloat32);
    torch::Tensor Initial = initial.to(torch::kFloat32);
    Initial = Initial.to(Target.device());

    int Height = Target.size(0);
    int Width  = Target.size(1);

    // Define phase-normalization function
    auto phase = [](const torch::Tensor& p) {
        return p / (p.abs() + 1e-8);
    };

    // Initialize with random phase
    auto rand_phase = torch::rand({Height, Width}, torch::kFloat32) * 2 * M_PI;
    rand_phase = rand_phase.to(Target.device());
    auto Object = Initial * torch::polar(torch::ones_like(rand_phase), rand_phase); // magnitude 1, phase = rand_phase

    for (int i = 0; i < iterations; ++i) {
        auto U  = torch::fft::ifft2(Object);
        auto Up = Target * phase(U);
        auto D  = torch::fft::fft2(Up);
        Object  = Initial * phase(D);
    }

    std::cout << "INFO: [GSAlgorithm] Completed " << iterations << " iterations of Gerchberg-Saxton algorithm.\n";
    return torch::angle(Object);
}
