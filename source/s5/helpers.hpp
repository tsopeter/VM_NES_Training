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
#include "../s2/plm_device.hpp"

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
    int index;
    int 

    _Result ();
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
    ~Parameters ();

    // Number of samples
    int64_t n_samples = 200;
    int     n_epochs  = 50;

    int     epoch_counter = 0;


    int     upscale_amount       = 1;
    int     n_iterate_amount     = 4;

    int64_t steps                = 0;
    bool    flip_input_V         = false;
    bool    flip_input_H         = false;


    int     num_levels           = 16;
    PLM_Device_Enum plm_device_enum = PLM_Device_Enum::VISIBLE;
    // available levels: 2, 4, 8, 16

    std::vector<_Result> results = {};

    PDFunction process_fn;

    _Training Training;
    _Camera Camera;

    bool collect_data = false;
    std::string collect_data_directory = "./collected_data/";
    std::vector<torch::Tensor> collect_data_masks = {};

    void ExportResults (const std::string &filename, int mode=0, int dataset_size=0, int batch_size=0);
    std::vector<_Result> GetResults (int mode=0, int dataset_size=0, int batch_size=0);

    void SaveCollectedMasks ();

    moodycamel::ConcurrentQueue<torch::Tensor> masks_queue;
    std::atomic<bool> save_masks_thread_running {false};
    std::thread save_mask_thread;
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
    int padding = 0,
    int start_index = 0
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

void Set_SubTextureHook (
    Scheduler2 &,
    std::function<void(Shader[2], Texture[10], bool[10])> hook_function
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
