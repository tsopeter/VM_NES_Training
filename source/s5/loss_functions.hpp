#ifndef LOSS_FUNCTIONS_HPP
#define LOSS_FUNCTIONS_HPP

#include <torch/torch.h>

namespace LossFunctions {

torch::Tensor per_pixel_mse_loss (
    torch::Tensor &img,
    torch::Tensor &target
);

torch::Tensor ssim_loss (
    torch::Tensor &img,
    torch::Tensor &target,
    int window_size = 11,
    double sigma = 1.5,
    double C1 = 0.01 * 0.01,
    double C2 = 0.03 * 0.03
);


}

#endif