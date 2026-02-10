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
    // Create Gaussian window
    auto gauss = torch::exp(-torch::pow(torch::arange(window_size) - window_size / 2, 2) / (2 * sigma * sigma));
    gauss = gauss / gauss.sum();
    auto window = gauss.unsqueeze(0) * gauss.unsqueeze(1);
    window = window.to(img.device());

    // Compute means
    auto mu_img = torch::nn::functional::conv2d(img.unsqueeze(0).unsqueeze(0), window.unsqueeze(0).unsqueeze(0), {}, 1, window_size / 2);
    auto mu_target = torch::nn::functional::conv2d(target.unsqueeze(0).unsqueeze(0), window.unsqueeze(0).unsqueeze(0), {}, 1, window_size / 2);

    // Compute variances and covariance
    auto sigma_img = torch::nn::functional::conv2d(img.unsqueeze(0).unsqueeze(0) * img.unsqueeze(0).unsqueeze(0), window.unsqueeze(0).unsqueeze(0), {}, 1, window_size / 2) - mu_img * mu_img;
    auto sigma_target = torch::nn::functional::conv2d(target.unsqueeze(0).unsqueeze(0) * target.unsqueeze(0).unsqueeze(0), window.unsqueeze(0).unsqueeze(0), {}, 1, window_size / 2) - mu_target * mu_target;
    auto sigma_img_target = torch::nn::functional::conv2d(img.unsqueeze(0).unsqueeze(0) * target.unsqueeze(0).unsqueeze(0), window.unsqueeze(0).unsqueeze(0), {}, 1, window_size / 2) - mu_img * mu_target;

    // Compute SSIM
    auto ssim_map = ((2 * mu_img * mu_target + C1) * (2 * sigma_img_target + C2)) / ((mu_img * mu_img + mu_target * mu_target + C1) * (sigma_img + sigma_target + C2));
    
    return -ssim_map.mean();
}