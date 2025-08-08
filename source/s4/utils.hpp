#ifndef s4_utils_hpp__
#define s4_utils_hpp__

#include <torch/torch.h>
#include <raylib.h>

namespace s4_Utils
{
    /**
     * @brief GetRandomPhaseMask generates a deterministic phase mask of h, w.
     * 
     */
    torch::Tensor GetRandomPhaseMask (int64_t h, int64_t w);

    /**
     * @brief Transforms a Tensor to a Raylib Image
     * 
     */
    Image         TensorToImage      (torch::Tensor&);

    torch::Tensor GSAlgorithm (const torch::Tensor &target, int iterations);



} // namespace s4_Utils



#endif
