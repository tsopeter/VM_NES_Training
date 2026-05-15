#include "runner.hpp"
#include <fstream>
#include <sys/wait.h>
#include <fstream>
#include "raylib.h"

void Runner::Run (std::string config_file) {
    srand(42);
    torch::manual_seed(42);

    int epoch = 0;
    Scheduler2 scheduler;
    Model model;

    InitConfigKeyMap(); 

    model.init(ModelHeight, ModelWidth, scheduler.maximum_number_of_frames_in_image, ModelDistribution, params.num_levels);


    std::cout << "INFO: [Runner::Run] Setting up optimizer...\n";
    torch::optim::Adam adam_opt(model.parameters(), torch::optim::AdamOptions(params.Training.lr));
    s4_Optimizer optimizer(
        adam_opt,
        model
    );

    std::cout << "INFO: [Runner::Run] Setting up scheduler...\n";
    Helpers::Run::Setup_Scheduler(
        params,
        scheduler,
        optimizer,
        *model.m_dist,
        model.m_Height,
        model.m_Width
    );

    Helpers::Run::EvalFunctions eval_fn;

    eval_fn.sample = [&model](int i) -> torch::Tensor {
        return model.sample(i);
    };
    eval_fn.base = [&model](int i) -> torch::Tensor {
        return model.m_dist->base(i);
    };
    eval_fn.squash = [&model]() -> void {
        model.squash();
    };
    eval_fn.entropy = [&model]() -> double {
        auto ent = model.m_dist->entropy().mean();
        return ent.item<double>();
    };
    eval_fn.update = [&scheduler]() -> double {
        return scheduler.Update();
    };
    eval_fn.loss = [&scheduler]() -> double {
        return 0.0f; /* Not implemented yet */
    };

    scheduler.StopThreads();
    scheduler.StopCamera();
    scheduler.StopWindow();

}

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
    if (m_model_distribution == DistributionType::NORMAL2) {
        return {m_parameter, m_parameter_std};
    }
    else {
        return {m_parameter};
    }
}

torch::Tensor &Runner::Model::get_parameters () {
    return m_parameter;
}

int64_t Runner::Model::N_samples () const {
    return m_action.size(0);
}

void Runner::ParseConfigFile (const std::string &filename) {
    std::cout << "INFO: [Runner::ParseConfigFile] Parsing configuration file: " << filename << "...\n";
    std::ifstream ifs(filename);
    if (!ifs) {
        throw std::runtime_error("Runner::ParseConfigFile: Could not open " + filename);
    }

    std::string key;
    for (;ifs >> key;) {
        //std::cout << "INFO: [Runner::ParseConfigFile] Looking for key: " << key << " ...\n";
        for (const auto &entry : config_key_map) {
            //std::cout<<entry.name<<std::endl;
            if (key == entry.name) {
                entry.setter(ifs);
                break;
            }
        }
    }
}

void Runner::InitConfigKeyMap () {
    config_key_map = {
        {
            "Height",
            [this](std::ifstream &ifs) {
                int64_t Height;
                ifs >> Height;
                ModelHeight = Height;
                std::cout << "Setting Height...\n";
            }
        },
        {
            "Width",
            [this](std::ifstream &ifs) {
                int64_t Width;
                ifs >> Width;
                ModelWidth = Width;
                std::cout << "Setting Width...\n";
            }
        },
        {
            "Distribution",
            [this](std::ifstream &ifs) {
                std::string dist_str;
                ifs >> dist_str;
                if (dist_str == "normal") {
                    ModelDistribution = DistributionType::NORMAL;
                }
                else if (dist_str == "categorical") {
                    ModelDistribution = DistributionType::CATEGORICAL;
                } 
                else {
                    throw std::runtime_error("Runner::Run: Unsupported distribution type in config file.");
                }
                std::cout << "Setting Distribution...\n";
            }
        },
        {
            "LearningRate",
            [this](std::ifstream &ifs) {
                double lr;
                ifs >> lr;
                // Set learning rate in Training namespace
                params.Training.lr = lr;
                std::cout << "Setting Learning Rate...\n";
            }
        },
}