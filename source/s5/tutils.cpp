#include "tutils.hpp"

torch::Tensor tutils::_min_value (const torch::Tensor &t) {
    auto dtype = t.dtype();

    if (dtype == torch::kFloat16) {
        return torch::tensor(std::numeric_limits<float>::lowest(), torch::dtype(dtype));
    } else if (dtype == torch::kFloat32) {
        return torch::tensor(std::numeric_limits<float>::lowest(), torch::dtype(dtype));
    } else if (dtype == torch::kFloat64) {
        return torch::tensor(std::numeric_limits<double>::lowest(), torch::dtype(dtype));
    } else if (dtype == torch::kInt) {
        return torch::tensor(std::numeric_limits<int32_t>::min(), torch::dtype(dtype));
    } else if (dtype == torch::kInt64) {
        return torch::tensor(std::numeric_limits<int64_t>::min(), torch::dtype(dtype));
    } else if (dtype == torch::kByte) {
        return torch::tensor(std::numeric_limits<uint8_t>::min(), torch::dtype(dtype));
    } else {
        throw std::runtime_error("tutils::min_value: Unsupported dtype for min value retrieval.");
    }
}

torch::Tensor tutils::min_value (const torch::Tensor &t) {
    return tutils::_min_value(t).to(t.device());
}