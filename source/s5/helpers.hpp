#ifndef s5_helpers_hpp__
#define s5_helpers_hpp__

#define CHECKPOINT_DENOTE "#_Checkpoint_Config_File"

#include <torch/torch.h>
#include "raylib.h"

#include "../s2/dataloader.hpp"
#include "../s4/model.hpp"
#include "../s4/optimizer.hpp"
#include "../s4/utils.hpp"
#include "scheduler2.hpp"
#include "distributions.hpp"
#include "../s2/np2lt.hpp"

#include "../third-party/concurrentqueue.h"

#include <functional>
#include <iostream>
#include <atomic>

namespace Helpers {

struct _Training {
    double lr = 0.5;
    std::string dataset_path = "./Datasets/";
};

struct _Camera {
    bool partitioning = true;
    double exposure_time_us = 300.0f;
};

struct _Result {
    int label;
    int prediction;
    int index;
    int batch_id;
    double reward;

    _Result ();
};

struct _pdf {
    _pdf ();
    std::atomic<int64_t> correct = 0;
    std::atomic<int64_t> total   = 0;

    int64_t label_counts[10] = {0};
    int64_t label_freq[10] = {0};

    int64_t process_count = 0;
    int64_t save_iter     = 10'000;
    int min_limit = 20;

    int64_t accuracy_interval = 20;

    std::function<torch::Tensor(torch::Tensor&, torch::Tensor&)> loss_fn;

    torch::Tensor masks;
    torch::Tensor ratios;

    void clear_data ();
};

/**
 * Helpers::Parameters
 *
 * Used to define system parameters.
 *
 *
 */
struct Parameters {
    Parameters ();

    // Number of samples to use during training
    int64_t n_training_samples   = 1000;

    // Size of each batch; note that n_batch_size must evenly divide n_training_samples
   int64_t n_batch_size         = 20;

    // Number of samples per image; note that the actual number of samples is n_samples * 20
    int64_t n_samples            = 10;

    int64_t n_validation_samples = 1000;
    int64_t n_validation_batch_size = 1000;

    int64_t n_test_samples = 1000;
    int64_t n_test_batch_size = 1000;

    int64_t n_padding            = 0;
    float   sub_shader_threshold = 0.8;

    int     upscale_amount       = 1;
    int     n_iterate_amount     = 4;

    int64_t steps                = 0;
    bool    flip_input_V         = false;
    bool    flip_input_H         = false;

    std::vector<_Result> results = {};

    _pdf _PDF;

    PDFunction process_fn;

    _Training Training;
    _Camera Camera;

    void ExportResults (const std::string &filename, int mode=0);
    std::vector<_Result> GetResults (int mode=0);
};

/**
 * Helpers::Data
 * 
 * Used to create/load datasets into program.
 */
namespace Data {

/**
 * Global parameters
 *
 */

struct Batch {
    std::vector<Texture> textures;
    std::vector<int>     labels;
};

std::vector<Batch> Get (
    Parameters &,
    int n_data_points,
    int batch_size,
    s2_DataTypes dtype=s2_DataTypes::TRAIN,
    int padding = 0
);

std::vector<Batch> Get_Training (Parameters&);
std::vector<Batch> Get_Validation (Parameters &);
std::vector<Batch> Get_Test (Parameters &);
void               Delete (std::vector<Batch> &);

}

namespace Run {

void Setup_Scheduler (
    Parameters &,
    /* Scheduler used to coordinate system */
    Scheduler2 &,

    /* Optimizer */
    s4_Optimizer &,

    /* Distribution used by model */
    Distributions::Definition &,

    /* Model Parameter Height */
    int Height,

    /* Model Parameter Width */
    int Width
);

struct Performance {
    Performance ();
    double compute_time_s;

    int64_t samples_total;
    int64_t samples_correct;
    double  entropy;
    double  accuracy;

    double  loss;

    // Save performance metrics to file
    void Save (const std::string &filename, int epoch, std::string msg);

};

struct EvalFunctions {
    std::function<torch::Tensor(int)> sample;
    std::function<torch::Tensor(int)> base;
    std::function<void()>             squash;
    std::function<double()>           entropy;
    std::function<double()>           update;
    std::function<double()>           loss;
};

Performance Evaluate (
    Parameters &,
    Scheduler2 &,
    EvalFunctions &,
    Data::Batch &
);

Performance Evaluate (
    Parameters &,
    Scheduler2 &,
    EvalFunctions &,
    std::vector<Data::Batch> &
);

Performance Inference (
    Parameters &,
    Scheduler2 &,
    EvalFunctions &,
    Data::Batch &
);

Performance Inference (
    Parameters &,
    Scheduler2 &,
    EvalFunctions &,
    std::vector<Data::Batch> &
);


void Iterate (Parameters &, Scheduler2 &);

}

struct Checkpoint {
    Checkpoint ();
    int Epoch;
    std::string config_file;

    torch::Tensor mask;

    void Save (const std::string &directory);
};


}


#endif
