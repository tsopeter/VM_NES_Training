#include "remapper.hpp"

Remapper::Remapper () {
    // Default constructor
}

Remapper::Remapper (torch::Tensor map) {
    m_map = map;
}

Remapper::~Remapper () {
    // Destructor
}

torch::Tensor Remapper::remap (torch::Tensor input) {
    // Check if input is on the same device as m_map
    if (input.device() != m_map.device()) {
        throw std::runtime_error("Input tensor and map tensor must be on the same device.");
    }

    // Check if input is of type int64
    if (input.dtype() != torch::kInt64) {
        throw std::runtime_error("Input tensor must be of type int64.");
    }

    // Perform remapping using advanced indexing
    torch::Tensor output = m_map.index({input});

    return output;
}

void Remapper::set_map (torch::Tensor map) {
    m_map = map;
}