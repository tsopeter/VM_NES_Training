#include "quantize.hpp"
#include <cmath>

#include "../utils/utils.hpp"

Quantize::Quantize (int p_num_levels) {
    m_num_levels = p_num_levels;
    set_levels(p_num_levels);
    
}
Quantize::~Quantize () {}

torch::Tensor Quantize::operator()(torch::Tensor &x, bool arg) {
    if (x.device() != m_table.device())
        m_table = m_table.to(x.device()).to(x.dtype());

    auto normalized_x = (x + M_PI) / (2 * M_PI);   // [0,1]
    auto flat_x = normalized_x.view({-1, 1});                // [N,1]
    auto levels = m_table.view({1, -1});                       // [1,L]

    auto diffs = torch::remainder(flat_x - levels + 0.5, 1.0) - 0.5;  // periodic
    auto logits = -diffs.pow(2) / 1e-5;                      // sharpness factor
    auto weights = torch::softmax(logits, 1);                // [N,L]

    if (arg) {
        auto indices = std::get<1>(weights.max(1));          // [N]
        return indices.view_as(x).to(torch::kLong);
    }

    auto quantized = (weights * levels).sum(1);              // [N]
    auto quantized_phase = quantized * 2 * M_PI - M_PI;
    return quantized_phase.view_as(x);
}

torch::Tensor Quantize::operator[](const torch::Tensor &x) {
    if (x.device() != m_table.device())
        m_table = m_table.to(x.device()).to(x.dtype());

    if (x.device() != m_levels.device())
        m_levels = m_table.to(x.device()).to(x.dtype()).view({1, -1});

    auto normalized_x = (x + M_PI) / (2 * M_PI);  // [0, 1]
    auto flat_x = normalized_x.view({-1});        // [N]

    // [N, L] = [N, 1] - [1, L] with broadcasting
    auto t1 = Utils::GetCurrentTime_us();
    auto diffs = torch::remainder(flat_x.unsqueeze(1) - m_levels + 0.5, 1.0) - 0.5;
    Utils::SynchronizeCUDADevices();

    auto t2 = Utils::GetCurrentTime_us();

    // [N]
    auto indices = torch::argmin(diffs.abs(), 1);
    Utils::SynchronizeCUDADevices();
    auto t3 = Utils::GetCurrentTime_us();
    std::cout<<"INFO: [Quantize::operator[]] Remainder took: " << (t2 - t1) << " us\n";
    std::cout<<"INFO: [Quantize::operator[]] Argmin    took: " << (t3 - t2) << " us\n";

    return indices.view_as(x);
}

torch::Tensor Quantize::CPUOperator(const torch::Tensor &x) {
    TORCH_CHECK(!x.is_cuda(), "Input must be on CPU");

    if (m_table.device().is_cuda())
        m_table = m_table.cpu().to(x.dtype());

    auto levels = m_table.contiguous().view({-1});  // [L]
    auto normalized_x = (x + M_PI) / (2 * M_PI);     // [0, 1]
    auto flat_x = normalized_x.view({-1});           // [N]

    auto N = flat_x.size(0);
    auto L = levels.size(0);
    auto indices = torch::empty({N}, torch::TensorOptions().dtype(torch::kLong));

    auto flat_x_data = flat_x.data_ptr<float>();
    auto levels_data = levels.data_ptr<float>();
    auto indices_data = indices.data_ptr<int64_t>();

    for (int64_t i = 0; i < N; ++i) {
        float v = flat_x_data[i];
        float min_diff = 1.0f;
        int64_t min_idx = 0;
        for (int64_t j = 0; j < L; ++j) {
            float diff = std::remainder(v - levels_data[j], 1.0f);
            if (std::abs(diff) < std::abs(min_diff)) {
                min_diff = diff;
                min_idx = j;
            }
        }
        indices_data[i] = min_idx;
    }

    return indices.view_as(x);
}

void Quantize::set_levels(int num_levels) {
    // Based on m_num_levels, we modify table parameters to simulate different levels
    // We still have *16* inputs, but several map to the same level
    switch (num_levels) {
        case 2:
            m_table = torch::tensor({
                0.0000, 0.0000, 0.0000, 0.0000,
                0.0000, 0.0000, 0.0000, 0.0000,
                0.4916, 0.4916, 0.4916, 0.4916,
                0.4916, 0.4916, 0.4916, 0.4916
            }, torch::TensorOptions().dtype(torch::kFloat32));
            break;
        case 4:
            m_table = torch::tensor({
                0.0000, 0.0000, 0.0000, 0.0000,
                0.3426, 0.3426, 0.3426, 0.3426,
                0.4916, 0.4916, 0.4916, 0.4916,
                0.7970, 0.7970, 0.7970, 0.7970
            }, torch::TensorOptions().dtype(torch::kFloat32));
            break;
        case 8:
            m_table = torch::tensor({
                0.0000, 0.0000, 0.0205, 0.0205,
                0.0560, 0.0560, 0.1131, 0.1131,
                0.3426, 0.3426, 0.4228, 0.4228,
                0.5994, 0.5994, 0.7970, 0.7970
            }, torch::TensorOptions().dtype(torch::kFloat32));
            break;
        case 16:
            // Default table
            // do nothing
            break;
        default:
            throw std::runtime_error("Quantize: Unsupported number of levels.");
    };

    // Set m_levels
    m_levels = m_table.view({1, -1});
}