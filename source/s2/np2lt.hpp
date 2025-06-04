#ifndef s2_np2lt_hpp__
#define s2_np2lt_hpp__
#include <torch/torch.h>
#include <filesystem>

namespace np2lt {
    torch::Tensor f32(const std::filesystem::path& filename);
    torch::Tensor f64(const std::filesystem::path& filename);
    torch::Tensor i32(const std::filesystem::path& filename);
    torch::Tensor i64(const std::filesystem::path& filename);
    torch::Tensor u32(const std::filesystem::path& filename);
    torch::Tensor u64(const std::filesystem::path& filename);
};

#endif
