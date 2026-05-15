#include "runner.hpp"
#include <fstream>
#include <sys/wait.h>
#include <fstream>
#include "raylib.h"

void Runner::Run (std::string config_file) {
    srand(42);
    torch::manual_seed(42);

    InitConfigKeyMap(); 
    ParseConfigFile(config_file);

    model.init(model_height, model_width, params.burst_n, model_distribution, params.num_levels);

    std::cout << "INFO: [Runner::Run] Setting up optimizer...\n";
    torch::optim::Adam adam_opt(model.parameters(), torch::optim::AdamOptions(params.lr));
    s4_Optimizer optimizer(
        adam_opt,
        model
    );

    std::cout << "INFO: [Runner::Run] Setting up scheduler...\n";
    Helpers::Setup_Scheduler(
        params,
        scheduler,
        optimizer,
        *model.m_dist,
        model.m_Height,
        model.m_Width
    );

    Helpers::EvalFunctions eval_fn;

    eval_fn.sample = [this](int i) -> torch::Tensor {
        return model.sample(i);
    };
    eval_fn.base = [this](int i) -> torch::Tensor {
        return model.m_dist->base(i);
    };
    eval_fn.squash = [this]() -> void {
        model.squash();
    };
    eval_fn.entropy = [this]() -> double {
        auto ent = model.m_dist->entropy().mean();
        return ent.item<double>();
    };
    eval_fn.update = [this]() -> double {
        return scheduler.Update();
    };
    eval_fn.loss = [this]() -> double {
        return 0.0f; /* Not implemented yet */
    };

    for (int epoch = 0; epoch < params.n_epochs; ++epoch) {
        for (int step = 0; step < params.n_steps; ++step) {
            auto perf = Helpers::Run::Evaluate(params, scheduler, eval_fn);

            // Save performance to csv file
        }
    }

    scheduler.UnloadTextures();
    scheduler.StopThreads();
    scheduler.StopFPGA();
    scheduler.StopWindow();

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
                model_height = Height;
                std::cout << "Setting Height...\n";
            }
        },
        {
            "Width",
            [this](std::ifstream &ifs) {
                int64_t Width;
                ifs >> Width;
                model_width = Width;
                std::cout << "Setting Width...\n";
            }
        },
        {
            "Distribution",
            [this](std::ifstream &ifs) {
                std::string dist_str;
                ifs >> dist_str;
                if (dist_str == "normal") {
                    model_distribution = DistributionType::NORMAL;
                }
                else if (dist_str == "categorical") {
                    model_distribution = DistributionType::CATEGORICAL;
                } 
                else {
                    throw std::runtime_error("Runner::Run: Unsupported distribution type in config file.");
                }
                std::cout << "Setting Distribution...\n";
            }
        },
        {
            "Samples",
            [this](std::ifstream &ifs) {
                ifs >> params.n_samples;
                std::cout << "Setting Samples...\n";
            }
        },
        {
            "Upscale",
            [this](std::ifstream &ifs) {
                ifs >> params.upscale_amount;
                std::cout << "Setting Upscale...\n";
            }
        },
        {
            "Epochs",
            [this](std::ifstream &ifs) {
                ifs >> params.n_epochs;
                std::cout << "Setting Epochs...\n";
            }
        },
        {
            "Steps_Per_Epoch",
            [this](std::ifstream &ifs) {
                ifs >> params.n_steps;
                std::cout << "Setting Steps Per Epoch...\n";
            }
        },
        {
            "LearningRate",
            [this](std::ifstream &ifs) {
                ifs >> params.lr;
                std::cout << "Setting Learning Rate...\n";
            }
        },
        {
            "IterateN",
            [this](std::ifstream &ifs) {
                ifs >> params.n_iterate;
                std::cout << "Setting IterateN...\n";
            }
        },
        {
            "PLMDevice",
            [this](std::ifstream &ifs) {
                std::string device_str;
                ifs >> device_str;
                if (device_str == "visible") {
                    params.plm_device_enum = PLM_Device_Enum::VISIBLE;
                }
                else if (device_str == "nir") {
                    params.plm_device_enum = PLM_Device_Enum::NIR;
                }
                else {
                    throw std::runtime_error("Runner::Run: Unsupported PLM device type in config file.");
                }
                std::cout << "Setting PLM Device...\n";
            }
        },
        {
            "ADC_Delay_us",
            [this](std::ifstream &ifs) {
                ifs >> params.adc_delay_us;
                std::cout << "Setting ADC Delay (us)...\n";
            }
        },
        {
            "ADC_AverageN",
            [this](std::ifstream &ifs) {
                ifs >> params.adc_average_n;
                std::cout << "Setting ADC AverageN...\n";
            }
        },
        {
            "ADC_BurstN",
            [this](std::ifstream &ifs) {
                ifs >> params.adc_burst_n;
                std::cout << "Setting ADC BurstN...\n";
            }
        },
        {
            "ADC_Host_IP",
            [this](std::ifstream &ifs) {
                ifs >> params.adc_host_ip;
                std::cout << "Setting ADC Host IP...\n";
            }
        },
        {
            "ADC_Host_Port",
            [this](std::ifstream &ifs) {
                ifs >> params.adc_host_port;
                std::cout << "Setting ADC Host Port...\n";
            }
        }
    };
}