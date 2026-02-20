#include "loss_functions.hpp"

torch::Tensor LossFunctions::per_pixel_mse_loss (
    torch::Tensor &img,
    torch::Tensor &target
) {
    return -torch::mean((img - target) * (img - target));
}

torch::Tensor LossFunctions::ssim_loss (
    torch::Tensor &img,
    torch::Tensor &target,
    int window_size,
    double sigma,
    double C1,
    double C2
) {
    throw std::runtime_error("SSIM loss is not implemented yet.");
    return torch::tensor(0.0);
}