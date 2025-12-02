#include "optimizer.hpp"

s4_Optimizer::s4_Optimizer (torch::optim::Optimizer& opt, s4_Model& model)
: m_opt(opt), m_model(model)
{
    best_mask = torch::empty({});
    average_mask = torch::empty({});

    // Best reward would have the highest
    // possible value
    best_reward = -100000000;

}

s4_Optimizer::~s4_Optimizer () {

}

void s4_Optimizer::step_a (torch::Tensor &rewards) {
    if (rewards.size(0) != m_model.N_samples()) {
        throw std::runtime_error("Mismatch in reward size: got " + std::to_string(rewards.size(0)) +
                                 ", expected " + std::to_string(m_model.N_samples()));
    }

    int64_t m = m_model.N_samples() / 2;
    auto pos  = rewards.narrow(0, 0, m);
    auto neg  = rewards.narrow(0, m, m);

    auto pair = (pos + neg) / 2;
    rewards   = pair.repeat({2});

    auto sum_rewards = rewards.sum();
    auto baseline    = (sum_rewards - rewards)/(m_model.N_samples()-1);
    auto norm_sum    = (rewards - baseline)/rewards.std(true);

    auto logp        = m_model.logp_action();
    logp             = torch::sum(logp.view(std::vector<int64_t>{m_model.N_samples(), -1}), 1);
    auto loss        = -torch::mean(logp * norm_sum);

    m_opt.zero_grad();
    loss.backward();
    m_opt.step();

}

void s4_Optimizer::step_ppo (torch::Tensor &rewards) {
    if (rewards.size(0) != m_model.N_samples()) {
        throw std::runtime_error("Mismatch in reward size: got " + std::to_string(rewards.size(0)) +
                                 ", expected " + std::to_string(m_model.N_samples()));
    }

    // Rewards is of shape [N]

    auto norm_sum = (rewards - rewards.mean())/(rewards.std(true) + 1e-10); // [N]
    auto logp        = m_model.logp_action(); // [N, H, W]
    logp             = torch::mean(logp.view(std::vector<int64_t>{m_model.N_samples(), -1}), 1); // [N]

    // Init to zeros if not defined
    if (m_old_logp.defined() == false) {
        m_old_logp = logp.detach(); // no-op
    }

    // PPO
    auto ratio = torch::exp(logp - m_old_logp); // [N]

    auto clip  = torch::clamp(ratio, 1 - epsilon, 1 + epsilon); // [N]

    auto obj   = torch::min(ratio * norm_sum, clip * norm_sum); // [N]
    auto loss  = -torch::mean(obj); // [1]

    m_opt.zero_grad();
    loss.backward();
    m_opt.step();

    // Update old log probabilities
    m_old_logp = logp.detach();

}

void s4_Optimizer::step (torch::Tensor &rewards) {
    if (rewards.size(0) != m_model.N_samples()) {
        throw std::runtime_error("Mismatch in reward size: got " + std::to_string(rewards.size(0)) +
                                 ", expected " + std::to_string(m_model.N_samples()));
    }
    std::cout << "INFO: [s4_Optimizer::step] Rewards device: " << rewards.device() << '\n';

    auto u = utilities(rewards);

    // Best reward tracking
    if (rewards.max().item<double>() > best_reward) {
        best_reward = rewards.max().item<double>();

        // Get the index of the best reward
        auto max_idx = std::get<1>(rewards.max(0));

        // Get the action that corresponds to the best reward
        auto action = m_model.action(); // [N, H, W]
        best_mask = action.index_select(0, max_idx); // [H, W]
    }

    // Compute the average mask
    /*
    {
        auto action = m_model.action(); // [N, H, W]
        average_mask = action.mean(0); // [H, W]
    }
    */

    // If xnes_normal, we need to compute gradients differently
    if (m_model.get_definition()->get_name() == "xnes_normal") {
        // Update using sNES (does not use SGD or Adam)
        xNES_update(rewards);
        return;
    }

    std::cout << "INFO: [s4_Optimizer::step] u device: " << u.device() << '\n';

    auto logp        = m_model.logp_action();
    u                = u.to(logp.device());
    logp             = torch::sum((logp).view(std::vector<int64_t>{m_model.N_samples(), -1}), 1);

    auto loss        = -torch::mean(logp * u);

    m_opt.zero_grad();
    loss.backward();
    m_opt.step();

    // Detach logp to avoid memory leak ??
    // Maybe the optimizer holds onto the computation graph otherwise
    logp.detach();

}

void s4_Optimizer::step_fisher (torch::Tensor &rewards) {
    if (rewards.size(0) != m_model.N_samples()) {
        throw std::runtime_error("Mismatch in reward size: got " + std::to_string(rewards.size(0)) +
                                 ", expected " + std::to_string(m_model.N_samples()));
    }
    torch::NoGradGuard no_grad;

    // Compute by updating the model parameters directly.
    // Currently, only implemented for Categorical actions.

    

}

torch::Tensor s4_Optimizer::utilities (torch::Tensor &rewards) {
    std::cout << "INFO: [s4_Optimizer::utilities] Calculating utilities...\n";
    torch::NoGradGuard no_grad;
    int64_t N = rewards.size(0);
    auto order = std::get<1>(rewards.sort(/*dim=*/0, /*descending=*/true));
    auto ranks = torch::empty_like(order);
    auto arange = torch::arange(N, rewards.options()).to(torch::kLong);
    ranks.index_put_({order}, arange);

    auto denom = torch::log(torch::tensor(static_cast<double>(N) / 2.0 + 1.0, rewards.options()));
    auto util = denom - torch::log(ranks.to(rewards.dtype()) + 1.0);
    util = torch::clamp(util, /*min=*/0.0);

    util = util / util.sum();
    util = util - 1.0 / static_cast<double>(N);

    return util;
}

torch::Tensor s4_Optimizer::norm_reward (torch::Tensor &rewards) {
    auto mean = rewards.mean();
    auto std  = rewards.std(true);

    return (rewards - mean) / (std + 1e-10);
}

void s4_Optimizer::xNES_update (torch::Tensor &rewards) {
    auto u = utilities(rewards); // [N]

    // Get the learning rate from optimizer
    auto def = m_model.get_definition();

    // Get mu and std
    auto &mu  = def->mu ();  // [H, W]
    auto &std = def->std (); // [H, W]

    // Get the action
    auto action = m_model.action(); // [N, H, W]

    // Get the sample
    // Note that action = mu + eps * std
    // so eps = (action - mu) / std
    auto eps = (action - mu.unsqueeze(0)) / std.unsqueeze(0); // [N, H, W]

    // Expand u to shape [N, 1, 1]
    auto u_exp = -u.view({-1, 1, 1});

    // Compute the gradients directly
    auto nabla_J = std * torch::sum(u_exp * eps, 0);
    auto covGrad = torch::sum(u_exp * ((eps * eps) - 1), 0);
    auto nabla_K = torch::exp(0.5 * xNES_lr_std * covGrad);

    mu -= xNES_lr_mu * nabla_J;
    std *= nabla_K;
}