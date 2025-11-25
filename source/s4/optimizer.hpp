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
    void step_a (torch::Tensor &rewards);
    void step_ppo (torch::Tensor &rewards);
    void step_fisher (torch::Tensor &rewards);

    double epsilon = 0.1;
    double xNES_lr_mu = 0.1;
    double xNES_lr_std = 0.1;

    torch::Tensor best_mask;
    double best_reward;
private:
    torch::optim::Optimizer &m_opt;
    s4_Model& m_model;
    torch::Tensor m_old_logp;

    torch::Tensor utilities (torch::Tensor &rewards);
    torch::Tensor norm_reward (torch::Tensor &rewards);

    void xNES_update (torch::Tensor &rewards);
};

#endif
