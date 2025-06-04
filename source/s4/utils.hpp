#ifndef s4_utils_hpp__
#define s4_utils_hpp__

#include <torch/torch.h>

namespace s4_Utils
{
    torch::Tensor GetRandomPhaseMask (int64_t h, int64_t w);


} // namespace s4_Utils



#endif
