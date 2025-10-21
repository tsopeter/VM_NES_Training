#include "runner.hpp"
#include <fstream>

void Runner::Run () {
    std::cout << "INFO: [Runner::Run] Starting Runner...\n";
    ParseConfigFile("config.txt");

    std::string log_file = checkpoint_directory + "/a/mean_reward.txt";
    ExportConfig(log_file);

    Scheduler2 scheduler;

    Model model;
    model.init(ModelHeight, ModelWidth, scheduler.maximum_number_of_frames_in_image, ModelDistribution);
    torch::optim::Adam adam_opt(model.parameters(), torch::optim::AdamOptions(Helpers::Parameters::Training::lr));
    s4_Optimizer optimizer(
        adam_opt,
        model
    );

    Helpers::Run::Setup_Scheduler(
        scheduler,
        optimizer,
        *model.m_dist,
        model.m_Height,
        model.m_Width
    );

    auto train_data = Helpers::Data::Get_Training();
    auto val_data   = Helpers::Data::Get_Validation();

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

    for (int epoch = 0; epoch < n_epochs; ++epoch) {
        std::cout << "INFO: [Runner::Run] Starting Epoch " << epoch << "...\n";

        auto train_perf = Helpers::Run::Evaluate(
            scheduler,
            eval_fn,
            train_data
        );

        auto val_perf = Helpers::Run::Evaluate(
            scheduler,
            eval_fn,
            val_data
        );

        train_perf.Save(
            log_file,
            epoch,
            "Training Performance"
        );

        val_perf.Save(
            log_file,
            epoch,
            "Validation Performance"
        );
    }







    scheduler.StopThreads();
    scheduler.StopCamera();
    scheduler.StopWindow();

    Helpers::Data::Delete(train_data);
    Helpers::Data::Delete(val_data);
}

Runner::Model::Model () {}
Runner::Model::~Model () {
    if (m_dist != nullptr) {
        delete m_dist;
    }
}

void Runner::Model::init (int64_t Height, int64_t Width, int64_t n, DistributionType::Type dist_type) {
    m_Height = Height;
    m_Width  = Width;
    m_n      = n;

    // Initialize m_parameter based on distribution type
    if (dist_type == DistributionType::NORMAL) {
        m_parameter = torch::zeros({Height, Width}, torch::kFloat32);
        m_dist = new Distributions::Normal(m_parameter, 0.1);
    }
    else if (dist_type == DistributionType::CATEGORICAL) {
        m_parameter = torch::zeros({Height, Width, 16}, torch::kFloat32);
        m_dist = new Distributions::Categorical(m_parameter);
    }
    else if (dist_type == DistributionType::BINARY) {
        m_Height = 2 * Height;
        m_Width  = 2 * Width;
        m_parameter = torch::zeros({m_Height, m_Width, 2}, torch::kFloat32);
        m_dist = new Distributions::Binary(m_parameter);
    }
    else {
        throw std::runtime_error("Model::init: Unsupported distribution type.");
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

std::vector<torch::Tensor> Runner::Model::parameters () {
    return {m_parameter};
}

torch::Tensor &Runner::Model::get_parameters () {
    return m_parameter;
}

int64_t Runner::Model::N_samples () const {
    return m_action.size(0);
}

void Runner::ParseConfigFile (const std::string &filename) {
    std::ifstream ifs(filename);
    if (!ifs) {
        throw std::runtime_error("Runner::ParseConfigFile: Could not open " + filename);
    }

    std::string key;
    for (;ifs >> key;) {
        for (const auto &entry : config_key_map) {
            if (key == entry.name) {
                entry.setter(ifs);
                break;
            }
        }
    }
}

void Runner::ExportConfig (const std::string &filename) {
    std::ofstream ofs(filename);
    if (!ofs) {
        throw std::runtime_error("Runner::ExportConfig: Could not open " + filename + " for writing.");
    }

    // Print all dataset sizes

    ofs << "Dataset parameters: ";
    ofs << "\tData Padding Amount: ";
    ofs << Helpers::Parameters::n_padding << '\n';

    ofs << "\tSub Shader Threshold: ";
    ofs << Helpers::Parameters::sub_shader_threshold << '\n';

    ofs << "\tTraining Dataset:\n";
    ofs << "\t\tTraining Size: ";
    ofs << Helpers::Parameters::n_training_samples << '\n';

    ofs << "\t\tTraining Batch Size: ";
    ofs << Helpers::Parameters::n_batch_size << '\n';

    ofs << "\tValidation Dataset:\n";
    ofs << "\t\tValidation Size: ";
    ofs << Helpers::Parameters::n_validation_samples << '\n';

    ofs << "\t\tValidation Batch Size: ";
    ofs << Helpers::Parameters::n_validation_batch_size << '\n';

    ofs << "\tTest Dataset:\n";
    ofs << "\t\tTest Size: ";
    ofs << Helpers::Parameters::n_test_samples << '\n';

    ofs << "\t\tTest Batch Size: ";
    ofs << Helpers::Parameters::n_test_batch_size << '\n';

    ofs << "Model Parameters:\n";
    ofs << "\tModel Height: ";
    ofs << ModelHeight << '\n';

    ofs << "\tModel Width: ";
    ofs << ModelWidth << '\n';

    ofs << "\tModel Distribution: ";
    switch (ModelDistribution) {
        case DistributionType::NORMAL:
            ofs << "normal\n";
            break;
        case DistributionType::CATEGORICAL:
            ofs << "categorical\n";
            break;
        case DistributionType::BINARY:
            ofs << "binary\n";
            break;
        default:
            ofs << "unknown\n";
            break;
    }

    ofs << "\tUpscale Factor: ";
    ofs << Helpers::Parameters::upscale_amount << '\n';

    ofs << "Standard Deviation (if applicable): ";
    ofs << "Not specified\n";

    ofs << "Training Parameters:\n";
    ofs << "\tLearning Rate: ";
    ofs << Helpers::Parameters::Training::lr << '\n';

    ofs << "\tOptimizer: Adam\n";
    ofs << "\tSamples per Image: ";
    ofs << Helpers::Parameters::n_samples * 20 << '\n';

    ofs << "\tEpochs: ";
    ofs << n_epochs << '\n';

    ofs.close();
}