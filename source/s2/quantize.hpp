#pragma once
#include <torch/torch.h>

/*
Python Implementation of Quantization

self.normalized_phase_levels = torch.from_numpy(np.array([   \
            0.0000, 0.0100, 0.0205, 0.0422, 
            0.0560, 0.0727, 0.1131, 0.1734, 
            0.3426, 0.3707, 0.4228, 0.4916, 
            0.5994, 0.6671, 0.7970, 0.9375,
        ])).to(device) # 0 to 2 pi -> 0 to 1
...
def quantize(self, X: torch.Tensor) -> torch.Tensor:
        Normalized_X = (X + torch.pi) / (2 * torch.pi)  # [N, B, 1]
        flat_X = Normalized_X.view(-1, 1)               # [N*B, 1]
        levels = self.normalized_phase_levels.to(X.device, X.dtype)  # [1, L]

        diffs = torch.remainder(flat_X - levels + 0.5, 1.0) - 0.5
        logits = -diffs**2 / 1e-5
        weights = torch.softmax(logits, dim=1)                        # [N*B, L]

        quantized = (weights * levels).sum(dim=1)                     # [N*B]
        quantized_phase = quantized * 2 * torch.pi - torch.pi        # [N*B]

        return quantized_phase.view(X.shape)
*/

class Quantize {
public:
    /**
     * @brief Quantizes input `x` based on PLM quantization table provided by Texas Instruments.
     * 
     * 
     * Code snippet:
     * 
     *      auto q = Quantize();
     *      auto y = q(x);
     */
    Quantize(int num_levels=16);
    ~Quantize();

    void set_levels(int num_levels);

    /**
     * @brief Returns quantized value (index if arg is true).
     * Inputs are expected to be -pi to pi
     * 
     */
    torch::Tensor operator()(torch::Tensor &x, bool arg=false); 

    /**
     * @brief Uses hard quantization (not differentiable)
     * Inputs are expected to be -pi to pi
     */
    torch::Tensor operator[](const torch::Tensor &x);
    torch::Tensor CPUOperator(const torch::Tensor &x);
private:
    torch::Tensor m_table = torch::tensor({
        0.0000, 0.0100, 0.0205, 0.0422,
        0.0560, 0.0727, 0.1131, 0.1734,
        0.3426, 0.3707, 0.4228, 0.4916,
        0.5994, 0.6671, 0.7970, 0.9375
    }, torch::TensorOptions().dtype(torch::kFloat32));  // Or use kFloat32 if needed

    torch::Tensor m_levels = torch::tensor({
        0.0000,
        0.0000,
        0.0000,
        0.0000,
        0.0000,
        0.0000,
        0.0000,
        0.0000,
        0.0000,
        0.0000,
        0.0000,
        0.0000,
        0.0000,
        0.0000,
        0.0000,
        0.0000
    }, torch::TensorOptions().dtype(torch::kFloat32));

    int m_num_levels;
};