#include "runner.hpp"
#include <fstream>

void Runner::Run (std::string config_file) {
    Pylon::PylonAutoInitTerm init {};

    int epoch = 0;
    Scheduler2 scheduler;
    Model model;

    std::cout << "INFO: [Runner::Run] Starting Runner...\n";
    InitConfigKeyMap(); 
    ParseConfigFile(config_file);

    std::cout << "INFO: [Runner::Run] Exporting configuration to log file...\n";
    std::string log_file = checkpoint_directory + "/a/mean_reward.txt";
    ExportConfig(log_file);

    if (!m_load_checkpoint) {
        epoch = 0;
        model.init(ModelHeight, ModelWidth, scheduler.maximum_number_of_frames_in_image, ModelDistribution);
    }
    else {
        LoadCheckpointFile(config_file);
        epoch = m_checkpoint_epoch;
        model.init(m_checkpoint_mask, ModelDistribution);
    }
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

    auto train_data = Helpers::Data::Get_Training(params);
    auto val_data   = Helpers::Data::Get_Validation(params);

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

    for (; epoch < n_epochs; ++epoch) {
        std::cout << "INFO: [Runner::Run] Starting Epoch " << epoch << "...\n";

        auto train_perf = Helpers::Run::Evaluate(
            params,
            scheduler,
            eval_fn,
            train_data
        );

        auto val_perf = Helpers::Run::Inference(
            params,
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

        // Save checkpoint
        Helpers::Checkpoint cp;
        cp.Epoch = epoch;
        cp.config_file = config_file;
        cp.mask = model.get_parameters().detach().cpu();
        std::string cp_dir = checkpoint_directory;
        cp.Save(cp_dir);
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

void Runner::Model::init (int64_t Height, int64_t Width, int64_t n, DistributionType dist_type) {
    m_Height = Height;
    m_Width  = Width;
    m_n      = n;

    // Initialize m_parameter based on distribution type
    if (dist_type == DistributionType::NORMAL) {
        m_parameter = torch::zeros({Height, Width}, torch::kFloat32).to(DEVICE);
        m_dist = new Distributions::Normal(m_parameter, 0.1);
    }
    else if (dist_type == DistributionType::CATEGORICAL) {
        m_parameter = torch::zeros({Height, Width, 16}, torch::kFloat32).to(DEVICE);
        m_dist = new Distributions::Categorical(m_parameter);
    }
    else if (dist_type == DistributionType::BINARY) {
        m_Height = 2 * Height;
        m_Width  = 2 * Width;
        m_parameter = torch::zeros({m_Height, m_Width, 2}, torch::kFloat32).to(DEVICE);
        m_dist = new Distributions::Binary(m_parameter);
    }
    else {
        throw std::runtime_error("Model::init: Unsupported distribution type.");
    }
    m_parameter.set_requires_grad(true);
}

void Runner::Model::init (torch::Tensor tensor, DistributionType dist_type) {
    m_parameter = tensor.to(DEVICE);
    m_parameter.set_requires_grad(true);

    // Initialize m_dist based on distribution type
    if (dist_type == DistributionType::NORMAL) {
        m_dist = new Distributions::Normal(m_parameter, 0.1);
    }
    else if (dist_type == DistributionType::CATEGORICAL) {
        m_dist = new Distributions::Categorical(m_parameter);
    }
    else if (dist_type == DistributionType::BINARY) {
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
    std::cout << "INFO: [Runner::ParseConfigFile] Parsing configuration file: " << filename << "...\n";
    std::ifstream ifs(filename);
    if (!ifs) {
        throw std::runtime_error("Runner::ParseConfigFile: Could not open " + filename);
    }

    std::string key;
    for (;ifs >> key;) {
        std::cout << "INFO: [Runner::ParseConfigFile] Looking for key: " << key << " ...\n";
        for (const auto &entry : config_key_map) {
            std::cout<<entry.name<<std::endl;
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

    ofs << "Dataset parameters:\n";
    ofs << "\tData Padding Amount: ";
    ofs << params.n_padding << '\n';

    ofs << "\tSub Shader Threshold: ";
    ofs << params.sub_shader_threshold << '\n';

    ofs << "\tTraining Dataset:\n";
    ofs << "\t\tTraining Size: ";
    ofs << params.n_training_samples << '\n';

    ofs << "\t\tTraining Batch Size: ";
    ofs << params.n_batch_size << '\n';

    ofs << "\tValidation Dataset:\n";
    ofs << "\t\tValidation Size: ";
    ofs << params.n_validation_samples << '\n';

    ofs << "\t\tValidation Batch Size: ";
    ofs << params.n_validation_batch_size << '\n';

    ofs << "\tTest Dataset:\n";
    ofs << "\t\tTest Size: ";
    ofs << params.n_test_samples << '\n';

    ofs << "\t\tTest Batch Size: ";
    ofs << params.n_test_batch_size << '\n';

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
    ofs << params.upscale_amount << '\n';

    ofs << "Standard Deviation (if applicable): ";
    ofs << "Not specified\n";

    ofs << "Training Parameters:\n";
    ofs << "\tLearning Rate: ";
    ofs << params.Training.lr << '\n';

    ofs << "\tOptimizer: Adam\n";
    ofs << "\tSamples per Image: ";
    ofs << params.n_samples * 20 << '\n';

    ofs << "\tEpochs: ";
    ofs << n_epochs << '\n';

    ofs.close();
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
                else if (dist_str == "binary") {
                    ModelDistribution = DistributionType::BINARY;
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
        {
            "TrainingSamples",
            [this](std::ifstream &ifs) {
                int64_t n_samples;
                ifs >> n_samples;
                // Set number of training samples in Helpers::Parameters namespace
                params.n_training_samples = n_samples;
                std::cout << "Setting Training Samples...\n";
            }
        },
        {
            "TrainingBatchSize",
            [this](std::ifstream &ifs) {
                int64_t batch_size;
                ifs >> batch_size;
                // Set training batch size in Helpers::Parameters namespace
                params.n_batch_size = batch_size;
                std::cout << "Setting Training Batch Size...\n";
            }
        },
        {
            "ValidationSamples",
            [this](std::ifstream &ifs) {
                int64_t n_val_samples;
                ifs >> n_val_samples;
                // Set number of validation samples in Helpers::Parameters namespace
                params.n_validation_samples = n_val_samples;
                std::cout << "Setting Validation Samples...\n";
            }
        },
        {
            "ValidationBatchSize",
            [this](std::ifstream &ifs) {
                int64_t val_batch_size;
                ifs >> val_batch_size;
                // Set validation batch size in Helpers::Parameters namespace
                params.n_validation_batch_size = val_batch_size;
                std::cout << "Setting Validation Batch Size...\n";
            }
        },
        {
            "TestSamples",
            [this](std::ifstream &ifs) {
                int64_t n_test_samples;
                ifs >> n_test_samples;
                // Set number of test samples in Helpers::Parameters namespace
                params.n_test_samples = n_test_samples;
                std::cout << "Setting Test Samples...\n";
            }
        },
        {
            "TestBatchSize",
            [this](std::ifstream &ifs) {
                int64_t test_batch_size;
                ifs >> test_batch_size;
                // Set test batch size in Helpers::Parameters namespace
                params.n_test_batch_size = test_batch_size;
                std::cout << "Setting Test Batch Size...\n";
            }
        },
        {
            "Samples",
            [this](std::ifstream &ifs) {
                int64_t n;
                ifs >> n;
                // Set number of samples in Helpers::Parameters namespace
                params.n_samples = n;
                std::cout << "Setting Samples...\n";
            }
        },
        {
            "Padding",
            [this](std::ifstream &ifs) {
                int padding;
                ifs >> padding;
                // Set padding in Helpers::Parameters namespace
                params.n_padding = padding;
                std::cout << "Setting Padding...\n";
            }
        },
        {
            "SubShaderThreshold",
            [this](std::ifstream &ifs) {
                float threshold;
                ifs >> threshold;
                // Set sub shader threshold in Helpers::Parameters namespace
                params.sub_shader_threshold = threshold;
                std::cout << "Setting Sub Shader Threshold...\n";
            }
        },
        {
            "UpscaleFactor",
            [this](std::ifstream &ifs) {
                int upscale;
                ifs >> upscale;
                // Set upscale amount in Helpers::Parameters namespace
                params.upscale_amount = upscale;
                std::cout << "Setting Upscale Factor...\n";
            }
        },
        {
            "IterationAmount",
            [this](std::ifstream &ifs) {
                int n_iterate;
                ifs >> n_iterate;
                // Set number of iterate amount in Helpers::Parameters namespace
                params.n_iterate_amount = n_iterate;
                std::cout << "Setting Iteration Amount...\n";
            }
        },
        {
            "RegionFile",
            [this](std::ifstream &ifs) {
                std::string region_file;
                ifs >> region_file;
                // Set region file in Helpers::Parameters namespace
                params._PDF.masks = np2lt::f32(region_file).to(DEVICE);
                std::cout << "Setting Region File...\n";
            }
        },
        {
            "MinLimit",
            [this](std::ifstream &ifs) {
                int min_limit;
                ifs >> min_limit;
                // Set min_limit in Helpers::Parameters namespace
                params._PDF.min_limit = min_limit;
                std::cout << "Setting Min Limit...\n";
            }
        },
        {
            "SaveInterval",
            [this](std::ifstream &ifs) {
                int64_t save_interval;
                ifs >> save_interval;
                // Set save interval in params._PDF namespace
                params._PDF.save_iter = save_interval;
                std::cout << "Setting Save Interval...\n";
            }
        },
        {
            "CheckpointDirectory",
            [this](std::ifstream &ifs) {
                ifs >> checkpoint_directory;
                // Set checkpoint directory in Helpers::Parameters namespace
                std::cout << "Setting Checkpoint Directory...\n";
            }
        },
        {
            "ExposureTime",
            [this](std::ifstream &ifs) {
                int exposure_time;
                ifs >> exposure_time;
                // Set exposure time in Camera namespace
                params.Camera.exposure_time_us = exposure_time;
                std::cout << "Setting Exposure Time...\n";
            }
        },
        {
            "DatasetPath",
            [this](std::ifstream &ifs) {
                std::string path;
                ifs >> path;
                // Set dataset path in Helpers::Data namespace
                params.Training.dataset_path = path;
                std::cout << "Setting Dataset Path...\n";
            }
        },
        {
            "MinLimit",
            [this](std::ifstream &ifs) {
                int min_limit;
                ifs >> min_limit;
                // Set min_limit in Helpers::Parameters namespace
                params._PDF.min_limit = min_limit;
                std::cout << "Setting Min Limit...\n";
            }
        },
        {
            "Epochs",
            [this](std::ifstream &ifs) {
                ifs >> n_epochs;
                // Set number of epochs in Helpers::Parameters namespace
                std::cout << "Setting Epochs...\n";
            }
        },
        {
            CHECKPOINT_DENOTE,
            [this](std::ifstream &ifs) {
                m_load_checkpoint = true;
                std::cout << "Detected Checkpoint Config File...\n";
            }
        }
    };
}

void Runner::LoadCheckpointFile (const std::string &config_file) {
    std::ifstream ifs(config_file);
    if (!ifs) {
        throw std::runtime_error("Runner::LoadCheckpointFile: Could not open " + config_file);
    }

    std::string key;
    for (;ifs >> key;) {
        if (key == "CheckpointEpoch") {
            ifs >> m_checkpoint_epoch;
        }
        else if (key == "MaskFile") {
            std::string mask_file;
            ifs >> mask_file;
            torch::load(m_checkpoint_mask, mask_file);
        }
    }

}