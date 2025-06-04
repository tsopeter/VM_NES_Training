#include "utils.hpp"

// Deterministic random phase mask in [-pi, pi)
torch::Tensor s4_Utils::GetRandomPhaseMask (int64_t h, int64_t w) {
    torch::manual_seed(42);  // Fixed seed for determinism
    return torch::rand({h, w}) * 2 * M_PI - M_PI;
}