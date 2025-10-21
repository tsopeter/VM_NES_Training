#ifndef s5_tutils_hpp__
#define s5_tutils_hpp__

#include <torch/torch.h>
#include <numeric>

namespace tutils {

torch::Tensor min_value (const torch::Tensor &);
torch::Tensor _min_value (const torch::Tensor &);

}

#endif