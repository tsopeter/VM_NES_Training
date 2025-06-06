#ifndef s2_dist_hpp__
#define s2_dist_hpp__

#include <torch/torch.h>

class Dist {
public:
    virtual ~Dist() = default;
    virtual torch::Tensor sample(int) = 0;
    virtual torch::Tensor log_prob(torch::Tensor&) = 0;
};

#endif
