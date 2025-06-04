#include "quantize.hpp"
#include <cmath>

Quantize::Quantize () {}
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

    auto normalized_x = (x + M_PI) / (2 * M_PI);  // [0, 1]
    auto flat_x = normalized_x.view({-1});        // [N]
    auto levels = m_table.view({1, -1});          // [1, L]

    auto diffs = torch::remainder(flat_x.unsqueeze(1) - levels + 0.5, 1.0) - 0.5;
    auto indices = std::get<1>(diffs.abs().min(1));  // [N]

    return indices.view_as(x).to(torch::kLong);
}