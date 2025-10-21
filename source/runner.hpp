#ifndef runner_hpp__
#define runner_hpp__

#include "s5/scheduler2.hpp"
#include "s4/model.hpp"
#include "s4/optimizer.hpp"
#include "s5/distributions.hpp"
#include "s5/helpers.hpp"
#include <iostream>
#include <fstream>
#include <ostream>

namespace Runner {

namespace DistributionType {
    enum Type {
        NORMAL,
        CATEGORICAL,
        BINARY
    };
}

static int ModelHeight = 400;
static int ModelWidth  = 640;
static int n_epochs    = 50;
DistributionType::Type ModelDistribution = DistributionType::NORMAL;
std::string checkpoint_directory = "./checkpoints/";

struct ConfigKeyMap {
    std::string      name;
    std::function<void(std::ifstream&)> setter;
};

static ConfigKeyMap config_key_map[] = {
    {
        "Height",
        [](std::ifstream &ifs) {
            int64_t Height;
            ifs >> Height;
            ModelHeight = Height;
        }
    },
    {
        "Width",
        [](std::ifstream &ifs) {
            int64_t Width;
            ifs >> Width;
            ModelWidth = Width;
        }
    },
    {
        "Distribution",
        [](std::ifstream &ifs) {
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
        }
    },
    {
        "LearningRate",
        [](std::ifstream &ifs) {
            double lr;
            ifs >> lr;
            // Set learning rate in Training namespace
            Helpers::Parameters::Training::lr = lr;
        }
    },
    {
        "TrainingSamples",
        [](std::ifstream &ifs) {
            int64_t n_samples;
            ifs >> n_samples;
            // Set number of training samples in Helpers::Parameters namespace
            Helpers::Parameters::n_training_samples = n_samples;
        }
    },
    {
        "TrainingBatchSize",
        [](std::ifstream &ifs) {
            int64_t batch_size;
            ifs >> batch_size;
            // Set training batch size in Helpers::Parameters namespace
            Helpers::Parameters::n_batch_size = batch_size;
        }
    },
    {
        "ValidationSamples",
        [](std::ifstream &ifs) {
            int64_t n_val_samples;
            ifs >> n_val_samples;
            // Set number of validation samples in Helpers::Parameters namespace
            Helpers::Parameters::n_validation_samples = n_val_samples;
        }
    },
    {
        "ValidationBatchSize",
        [](std::ifstream &ifs) {
            int64_t val_batch_size;
            ifs >> val_batch_size;
            // Set validation batch size in Helpers::Parameters namespace
            Helpers::Parameters::n_validation_batch_size = val_batch_size;
        }
    },
    {
        "TestSamples",
        [](std::ifstream &ifs) {
            int64_t n_test_samples;
            ifs >> n_test_samples;
            // Set number of test samples in Helpers::Parameters namespace
            Helpers::Parameters::n_test_samples = n_test_samples;
        }
    },
    {
        "TestBatchSize",
        [](std::ifstream &ifs) {
            int64_t test_batch_size;
            ifs >> test_batch_size;
            // Set test batch size in Helpers::Parameters namespace
            Helpers::Parameters::n_test_batch_size = test_batch_size;
        }
    },
    {
        "Samples",
        [](std::ifstream &ifs) {
            int64_t n;
            ifs >> n;
            // Set number of samples in Helpers::Parameters namespace
            Helpers::Parameters::n_samples = n;
        }
    },
    {
        "Padding",
        [](std::ifstream &ifs) {
            int padding;
            ifs >> padding;
            // Set padding in Helpers::Parameters namespace
            Helpers::Parameters::n_padding = padding;
        }
    },
    {
        "SubShaderThreshold",
        [](std::ifstream &ifs) {
            float threshold;
            ifs >> threshold;
            // Set sub shader threshold in Helpers::Parameters namespace
            Helpers::Parameters::sub_shader_threshold = threshold;
        }
    },
    {
        "UpscaleFactor",
        [](std::ifstream &ifs) {
            int upscale;
            ifs >> upscale;
            // Set upscale amount in Helpers::Parameters namespace
            Helpers::Parameters::upscale_amount = upscale;
        }
    },
    {
        "IterationAmount",
        [](std::ifstream &ifs) {
            int n_iterate;
            ifs >> n_iterate;
            // Set number of iterate amount in Helpers::Parameters namespace
            Helpers::Parameters::n_iterate_amount = n_iterate;
        }
    },
    {
        "RegionFile",
        [](std::ifstream &ifs) {
            std::string region_file;
            ifs >> region_file;
            // Set region file in Helpers::Parameters namespace
            Helpers::Parameters::_PDF::masks = np2lt::f32(region_file);
        }
    },
    {
        "MinLimit",
        [](std::ifstream &ifs) {
            int min_limit;
            ifs >> min_limit;
            // Set min_limit in Helpers::Parameters namespace
            Helpers::Parameters::_PDF::min_limit = min_limit;
        }
    },
    {
        "SaveInterval",
        [](std::ifstream &ifs) {
            int64_t save_interval;
            ifs >> save_interval;
            // Set save interval in Helpers::Parameters::_PDF namespace
            Helpers::Parameters::_PDF::save_iter = save_interval;
        }
    },
    {
        "CheckpointDirectory",
        [](std::ifstream &ifs) {
            ifs >> checkpoint_directory;
            // Set checkpoint directory in Helpers::Parameters namespace
        }
    },
    {
        "ExposureTime",
        [](std::ifstream &ifs) {
            int exposure_time;
            ifs >> exposure_time;
            // Set exposure time in Camera namespace
            Helpers::Parameters::Camera::exposure_time_us = exposure_time;
        }
    },
    {
        "DatasetPath",
        [](std::ifstream &ifs) {
            std::string path;
            ifs >> path;
            // Set dataset path in Helpers::Data namespace
            Helpers::Data::dataset_path = path;
        }
    },
    {
        "MinLimit",
        [](std::ifstream &ifs) {
            int min_limit;
            ifs >> min_limit;
            // Set min_limit in Helpers::Parameters namespace
            Helpers::Parameters::_PDF::min_limit = min_limit;
        }
    },
    {
        "Epochs",
        [](std::ifstream &ifs) {
            ifs >> n_epochs;
            // Set number of epochs in Helpers::Parameters namespace
        }
    }
};


void ParseConfigFile (const std::string &filename);
void ExportConfig    (const std::string &filename);

class Model : public s4_Model {
public:
    Model ();
    ~Model () override;

    void init (int64_t Height, int64_t Width, int64_t n, DistributionType::Type dist_type);
    torch::Tensor sample (int n);
    void squash ();
    torch::Tensor logp_action () override;
    std::vector<torch::Tensor> parameters ();
    torch::Tensor &get_parameters ();
    int64_t N_samples () const override;



    Distributions::Definition *m_dist = nullptr;
    torch::Tensor m_parameter;
    torch::Tensor m_action;
    std::vector<torch::Tensor> m_action_s;
    int64_t m_Height;
    int64_t m_Width;
    int64_t m_n;
    double std; /* May be unused */

};

void Run ();

};


#endif
