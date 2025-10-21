#include "optimizer.hpp"

s4_Optimizer::s4_Optimizer (torch::optim::Optimizer& opt, s4_Model& model)
: m_opt(opt), m_model(model)
{

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
    auto logp        = m_model.logp_action();
    logp             = torch::sum(logp.view(std::vector<int64_t>{m_model.N_samples(), -1}), 1);

    auto sum_rewards = rewards.sum();
    auto baseline    = (sum_rewards - rewards)/(m_model.N_samples()-1);
    auto norm_sum    = (rewards - baseline)/rewards.std(true);

    auto loss        = -torch::mean(logp * norm_sum);

    m_opt.zero_grad();
    loss.backward();
    m_opt.step();


}