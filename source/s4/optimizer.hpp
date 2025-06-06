#ifndef s4_optimizer_hpp__
#define s4_optimizer_hpp__

#include <torch/torch.h>
#include "../s2/dist.hpp"   /* Distribution used */
#include "model.hpp"        /* Model */

class s4_Optimizer {
public:
    s4_Optimizer (torch::optim::Optimizer&, s4_Model&);
    ~s4_Optimizer ();

    void step (torch::Tensor &rewards);

private:
    torch::optim::Optimizer &m_opt;
    s4_Model& m_model;
};

#endif
