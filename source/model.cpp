#include "runner.hpp"

// Implements the model


Runner::Model::Model () {
    m_dist = nullptr;
}
Runner::Model::~Model () {
    if (m_dist != nullptr) {
        delete m_dist;
    }
}

void Runner::Model::set_definition (Distributions::Definition* def) {
    if (m_dist != nullptr) {
        delete m_dist;
    }
    m_dist = def;
}

Distributions::Definition* Runner::Model::get_definition () {
    return m_dist;
}

void Runner::Model::init (int64_t Height, int64_t Width, int64_t n, DistributionType dist_type, int num_levels) {
    m_Height = Height;
    m_Width  = Width;
    m_n      = n;
    m_model_distribution = dist_type;

    // Initialize m_parameter based on distribution type
    if (dist_type == DistributionType::NORMAL) {
        m_parameter = torch::randn({Height, Width}, torch::kFloat32).to(DEVICE);
        m_dist = new Distributions::Normal(m_parameter, std);
        m_parameter.set_requires_grad(true);
    }
    else if (dist_type == DistributionType::CATEGORICAL) {
        m_parameter = torch::randn({Height, Width, num_levels}, torch::kFloat32).to(DEVICE);
        m_dist = new Distributions::Categorical(m_parameter);
        m_parameter.set_requires_grad(true);
    }
    else {
        throw std::runtime_error("Model::init: Unsupported distribution type.");
    }
}

void Runner::Model::init (torch::Tensor tensor, DistributionType dist_type) {
    m_parameter = tensor.to(DEVICE);
    m_parameter.set_requires_grad(true);

    // Initialize m_dist based on distribution type
    if (m_dist == nullptr) {
        if (dist_type == DistributionType::NORMAL) {
            m_dist = new Distributions::Normal(m_parameter, 0.1);
        }
        else if (dist_type == DistributionType::CATEGORICAL) {
            m_dist = new Distributions::Categorical(m_parameter);
        }
        else {
            throw std::runtime_error("Model::init: Unsupported distribution type.");
        }
    }
    else {
        auto &mu = m_dist->mu();
        mu = m_parameter; // set
    }
}

torch::Tensor Runner::Model::sample (int n) {
    torch::NoGradGuard no_grad;
    auto action = m_dist->sample(n); // [n, H, W] or [n, 2H, 2W]
    // Store into m_action_s
    m_action_s.push_back(action);
    return action;
}

void Runner::Model::squash () {
    if (m_action_s.empty()) return;

    m_action = torch::cat(m_action_s, 0);  // [k * n, H, W] or [k * n, 2H, 2W]
    m_action_s.clear();
}

torch::Tensor Runner::Model::logp_action () {
    return m_dist->log_prob(m_action);
}

torch::Tensor Runner::Model::action () {
    return m_action;
}

std::vector<torch::Tensor> Runner::Model::parameters () {
    return {m_parameter};
}

torch::Tensor &Runner::Model::get_parameters () {
    return m_parameter;
}

int64_t Runner::Model::N_samples () const {
    return m_action.size(0);
}
