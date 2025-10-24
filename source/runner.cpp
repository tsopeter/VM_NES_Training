#include "runner.hpp"
#include <fstream>
#include <sys/wait.h>
#include <fstream>
#include "raylib.h"

void Runner::Run (std::string config_file) {
    Pylon::PylonAutoInitTerm init {};

    int epoch = 0;
    Scheduler2 scheduler;
    Model model;
    std::cout << "INFO: [Runner::Run] Starting Runner...\n";
    InitConfigKeyMap(); 
    ParseConfigFile(config_file);
    PopulateNewDirectory(checkpoint_directory);
    model.std = m_std;

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

    //TestIfScreenIsOkay(params, scheduler); // Wait until screen is okay

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
    eval_fn.update = [&scheduler]() -> double {
        return scheduler.Update();
    };
    eval_fn.loss = [&scheduler]() -> double {
        return scheduler.Loss();
    };

    double previous_accuracy = 0.0f;
    for (; epoch < n_epochs; ++epoch) {
        std::cout << "INFO: [Runner::Run] Starting Epoch " << epoch << "...\n";

        // Save the previous model parameters, so if 
        // validation loss increases, we can revert back
        auto prev_params = model.get_parameters().detach().cpu();
        bool   revert = false;

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

        if (epoch == 0) {
            previous_accuracy = train_perf.accuracy;
        }

        // If the current accuracy is 20% less than previous accuracy, revert
        if (train_perf.accuracy < 0.8 * previous_accuracy) {
            std::cout << "WARNING: [Runner::Run] Training accuracy dropped from "
                      << previous_accuracy << " to " << train_perf.accuracy
                      << ". Reverting to previous model parameters.\n";
            model.init(prev_params.to(DEVICE), ModelDistribution);
            revert = true;
        }
        else {
            previous_accuracy = train_perf.accuracy;
        }

        Time current_time = GetCurrentTime();

        std::string train_perf_message = "Training\nEpoch " + std::to_string(epoch) + "\nTime: " + current_time.to_string();
        train_perf_message += (revert ? "\nModel parameters reverted due to increased training loss." : "");

        train_perf.Save(
            log_file,
            epoch,
            train_perf_message
        );

        std::string val_perf_message = "Validation\nEpoch " + std::to_string(epoch) + "\nTime: " + current_time.to_string();
        val_perf_message += (revert ? "\nModel parameters reverted due to increased training loss." : "");

        val_perf.Save(
            log_file,
            epoch,
            val_perf_message
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

void Runner::Inference (std::string config_file, s2_DataTypes data_type, int n_data_points) {
    Pylon::PylonAutoInitTerm init {};

    Scheduler2 scheduler;
    Model model;

    std::cout << "INFO: [Runner::Inference] Starting Inference...\n";
    InitConfigKeyMap(); 
    ParseConfigFile(config_file);

    if (!m_load_checkpoint) {
        throw std::runtime_error("Runner::Inference: Inference mode requires a checkpoint to be loaded.");
    }

    switch (data_type) {
        case s2_DataTypes::TRAIN:
            std::cout << "INFO: [Runner::Inference] Data Type: TRAIN\n";
            break;
        case s2_DataTypes::VALID:
            std::cout << "INFO: [Runner::Inference] Data Type: VALID\n";
            break;
        case s2_DataTypes::TEST:
            std::cout << "INFO: [Runner::Inference] Data Type: TEST\n";
            break;
        default:
            throw std::runtime_error("Runner::Inference: Unsupported data type for inference.");
    }

    std::cout << "INFO: [Runner::Inference] Loading checkpoint file...\n";
    LoadCheckpointFile(config_file);
    model.init(m_checkpoint_mask, ModelDistribution);

    torch::optim::Adam adam_opt(model.parameters(), torch::optim::AdamOptions(params.Training.lr));
    s4_Optimizer optimizer(
        adam_opt,
        model
    );

    std::cout << "INFO: [Runner::Inference] Setting up scheduler...\n";
    Helpers::Run::Setup_Scheduler(
        params,
        scheduler,
        optimizer,
        *model.m_dist,
        model.m_Height,
        model.m_Width
    );

    std::cout << "INFO: [Runner::Inference] Setting up dataset...\n";
    auto dataset = Helpers::Data::Get (
        params,
        n_data_points,
        n_data_points,
        data_type,
        params.n_padding
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
        return scheduler.Loss();
    };

    std::cout << "INFO: [Runner::Inference] Running inference...\n";
    auto test_perf = Helpers::Run::Inference(
        params,
        scheduler,
        eval_fn,
        dataset
    );

    // Export the test performance to results.txt stored in ./

    std::string msg;

    // message stores
    // the dataset used and the mask used
    switch (data_type) {
        case s2_DataTypes::TRAIN:
            msg += "Dataset: TRAIN\n";
            break;
        case s2_DataTypes::VALID:
            msg += "Dataset: VALID\n";
            break;
        case s2_DataTypes::TEST:
            msg += "Dataset: TEST\n";
            break;
        default:
            msg += "Dataset: UNKNOWN\n";
            break;
    }
    msg += "Mask Location: " + m_checkpoint_mask_location + "\n";


    test_perf.Save(
        "./results.txt",
        0,
        msg
    );

    scheduler.StopThreads();
    scheduler.StopCamera();
    scheduler.StopWindow();

    Helpers::Data::Delete(dataset);
}

void Runner::StaticInference (std::string config_file, s2_DataTypes data_type, int n_data_points) {
    Pylon::PylonAutoInitTerm init {};

    Scheduler2 scheduler;
    Model model;

    std::cout << "INFO: [Runner::Inference] Starting Inference...\n";
    InitConfigKeyMap(); 
    ParseConfigFile(config_file);

    switch (data_type) {
        case s2_DataTypes::TRAIN:
            std::cout << "INFO: [Runner::Inference] Data Type: TRAIN\n";
            break;
        case s2_DataTypes::VALID:
            std::cout << "INFO: [Runner::Inference] Data Type: VALID\n";
            break;
        case s2_DataTypes::TEST:
            std::cout << "INFO: [Runner::Inference] Data Type: TEST\n";
            break;
        default:
            throw std::runtime_error("Runner::Inference: Unsupported data type for inference.");
    }

    // Doesn't actually matter
    // as the mask is statically loaded onto PLM
    model.init(ModelHeight, ModelWidth, scheduler.maximum_number_of_frames_in_image, ModelDistribution);

    torch::optim::Adam adam_opt(model.parameters(), torch::optim::AdamOptions(params.Training.lr));
    s4_Optimizer optimizer(
        adam_opt,
        model
    );

    std::cout << "INFO: [Runner::Inference] Setting up scheduler...\n";
    Helpers::Run::Setup_Scheduler(
        params,
        scheduler,
        optimizer,
        *model.m_dist,
        model.m_Height,
        model.m_Width
    );
    scheduler.EnableStaticMode();

    std::cout << "INFO: [Runner::Inference] Setting up dataset...\n";
    auto dataset = Helpers::Data::Get (
        params,
        n_data_points,
        n_data_points,
        data_type,
        params.n_padding
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
        return scheduler.Loss();
    };

    std::cout << "INFO: [Runner::Inference] Running inference...\n";
    auto test_perf = Helpers::Run::Inference(
        params,
        scheduler,
        eval_fn,
        dataset
    );

    // Export the test performance to results.txt stored in ./

    std::string msg;

    // message stores
    // the dataset used and the mask used
    switch (data_type) {
        case s2_DataTypes::TRAIN:
            msg += "Dataset: TRAIN\n";
            break;
        case s2_DataTypes::VALID:
            msg += "Dataset: VALID\n";
            break;
        case s2_DataTypes::TEST:
            msg += "Dataset: TEST\n";
            break;
        default:
            msg += "Dataset: UNKNOWN\n";
            break;
    }
    msg += "Mask Location: " + m_checkpoint_mask_location + "\n";


    test_perf.Save(
        "./results.txt",
        0,
        msg
    );

    scheduler.StopThreads();
    scheduler.StopCamera();
    scheduler.StopWindow();

    Helpers::Data::Delete(dataset);

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
        m_dist = new Distributions::Normal(m_parameter, std);
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
            ofs << "\t\tStandard Deviation: " << m_std << '\n';
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
        },
        {
            "Std",
            [this](std::ifstream &ifs) {
                ifs >> m_std;
                std::cout << "Setting Standard Deviation...\n";
            }
        },
        {
            "InputFlipUD",
            [this](std::ifstream &ifs) {
                ifs >> params.flip_input_V;
                std::cout << "Setting Flip Input Vertical to " << (params.flip_input_V ? "true" : "false") << "...\n";
            }
        },
        {
            "InputFlipLR",
            [this](std::ifstream &ifs) {
                ifs >> params.flip_input_H;
                std::cout << "Setting Flip Input Horizontal to " << (params.flip_input_H ? "true" : "false") << "...\n";
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
            std::cout << "Loaded checkpoint epoch: " << m_checkpoint_epoch << "...\n";
        }
        else if (key == "MaskLocation") {
            ifs >> m_checkpoint_mask_location;
            std::cout << "Loading checkpoint mask from " << m_checkpoint_mask_location << "...\n";
            torch::load(m_checkpoint_mask, m_checkpoint_mask_location);
            std::cout << "Loaded checkpoint size: " << m_checkpoint_mask.sizes() << "...\n";
        } 
    }

}

void Runner::PopulateNewDirectory (const std::string &directory) {
    // This should be ran without sudo privileges
    // so create a fork

    if (fork() == 0) {
        // Child process
        // Remove privileges
        setuid(getuid());

        if (!std::filesystem::exists(directory)) {
            std::filesystem::create_directories(directory);
        }
        else {
            // just exit as it already exists
            exit(0);
        }

        // Create subdirectory a/
        std::string sub_directory = directory + "/a";;
        if (!std::filesystem::exists(sub_directory)) {
            std::filesystem::create_directories(sub_directory);
        }

        // Create log.txt and mean_reward.txt in /a/
        std::string log_path = sub_directory + "/log.txt";
        std::ofstream log_ofs(log_path);
        log_ofs.close();

        std::string reward_path = sub_directory + "/mean_reward.txt";
        std::ofstream reward_ofs(reward_path);
        reward_ofs.close();

        exit(0);

    }
    else {
        // Parent process
        int status;
        wait(&status);
        if (status != 0) {
            std::cerr << "ERROR: [Runner::PopulateNewDirectory] Child process failed to create directory.\n";
        }
    }
}

Runner::Time Runner::GetCurrentTime () {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm *parts = std::localtime(&now_c);

    Time time;
    time.hours = parts->tm_hour;
    time.minutes = parts->tm_min;
    time.seconds = parts->tm_sec;

    return time;
}

std::string Runner::Time::to_string () const {
    std::ostringstream oss;

    oss << (hours < 10 ? "0" : "") << hours << ":"
        << (minutes < 10 ? "0" : "") << minutes << ":"
        << (seconds < 10 ? "0" : "") << seconds;
    return oss.str();
}

bool Runner::TestIfScreenIsOkay (Helpers::Parameters &params, Scheduler2 &scheduler) {
    // Load in a test image
    Image test_image = LoadImage("source/Assets/test_image.png");
    Texture test_texture = LoadTextureFromImage(test_image);

    // The image is already 2560x1600,
    // we just need to display it to the screen


    while (true) {
        for (int i = 0; i < params.n_iterate_amount; ++i) {
            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexture(test_texture, 0, 0, WHITE);
            EndDrawing();

            scheduler.SetVSYNC_Marker();
            scheduler.WaitVSYNC_Diff(1);

            scheduler.SetLabel(0, 20); 
            // Doesn't matter, we
            // just need to display the image and
            // don't care about what the processor does with it
        }
        // Read from camera
        scheduler.ReadFromCamera();

        // Get the sample image from the scheduler
        auto sample = scheduler.GetSampleImage ();

        // 


    }
    // Wait a bit
    scheduler.SetVSYNC_Marker ();
    scheduler.WaitVSYNC_Diff (4);

    // Dump
    scheduler.Dump();

    UnloadImage(test_image);
    UnloadTexture(test_texture);
}