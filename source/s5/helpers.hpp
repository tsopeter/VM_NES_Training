#ifndef s5_helpers_hpp__
#define s5_helpers_hpp__

#include <torch/torch.h>
#include "raylib.h"

#include "../s2/dataloader.hpp"
#include "../s4/model.hpp"
#include "../s4/optimizer.hpp"
#include "../s4/utils.hpp"
#include "scheduler2.hpp"
#include "distributions.hpp"
#include "../s2/np2lt.hpp"

#include <functional>
#include <iostream>
#include <atomic>

namespace Helpers {

/**
 * Helpers::Parameters
 *
 * Used to define system parameters.
 *
 *
 */
namespace Parameters {

static bool inference = false;

//
// Note: training parameters are ignored when inference mode is used.

// Number of samples to use during training
static int64_t n_training_samples   = 1000;

// Size of each batch; note that n_batch_size must evenly divide n_training_samples
static int64_t n_batch_size         = 20;

// Number of samples per image; note that the actual number of samples is n_samples * 20
static int64_t n_samples            = 10;

static int64_t n_validation_samples = 1000;
static int64_t n_validation_batch_size = 1000;

static int64_t n_test_samples = 1000;
static int64_t n_test_batch_size = 1000;

static int64_t n_padding            = 0;
static float   sub_shader_threshold = 0.8;

static int     upscale_amount       = 1;
static int     n_iterate_amount     = 4;

static int64_t steps                = 0;

namespace _PDF {
    static std::atomic<double> correct = 0;
    static std::atomic<double> total   = 0;

    static int64_t label_counts[10] = {0};
    static int64_t label_freq[10] = {0};

    static int64_t process_count = 0;
    static int64_t save_iter     = 10'000;
    static int min_limit = 20;

    static int64_t accuracy_interval = 20;

    static std::function<torch::Tensor(torch::Tensor&, torch::Tensor&)> loss_fn = [](torch::Tensor &preds, torch::Tensor &targets) -> torch::Tensor {
        return -torch::nn::functional::cross_entropy(
            preds,
            targets,
            torch::nn::functional::CrossEntropyFuncOptions().reduction(torch::kNone)
        );
    };

    static torch::Tensor masks = np2lt::f32("source/Assets/regions2.npy");

    void clear_data ();
};

static PDFunction process_fn = [](CaptureData ts) -> std::pair<torch::Tensor, bool> {

    auto img = ts.full;
    auto l   = torch::tensor({ts.label});

    img      = torch::clamp(img, 0, 255).to(torch::kFloat32);
    img      = torch::where(img < _PDF::min_limit, torch::zeros_like(img), img).to(torch::kFloat64);

    img      = (img - _PDF::min_limit) * (255.0 / (255.0 - _PDF::min_limit));
    img      = torch::clamp(img, 0, 255) / 255.0f; // [0 - 1]

    auto q    = img.unsqueeze(0).unsqueeze(0); // [1, 1, H, W]
    auto sums = (q.unsqueeze(1) * _PDF::masks.to(img.device())).sum({2,3,4}); // [1, 10]

    if (_PDF::process_count % _PDF::save_iter == 0) {
        std::cout << "INFO: [process_fn] Processed " << _PDF::process_count << " samples so far.\n";
        auto img_img = img * 255.0f;
        img_img = img_img.contiguous().to(torch::kUInt8);

        auto Img = s4_Utils::TensorToImage(img_img);
        ExportImage(Img, "sample.bmp");
        UnloadImage(Img);
    }

    auto preds = sums.argmax(1); // [1]

    if (_PDF::process_count % _PDF::accuracy_interval == 0) {
        if (preds.item<int>() == ts.label) {
            _PDF::correct.fetch_add(1, std::memory_order_release);
        }
        _PDF::total.fetch_add(1, std::memory_order_release);

        _PDF::label_counts[ts.label]++;
        _PDF::label_freq[preds.item<int>()]++;
    }

    auto targets = l.to(torch::kLong).to(ts.full.device()); // [1]


    auto loss = _PDF::loss_fn(sums, targets);  // [1]

    ++_PDF::process_count;
    return {loss, true};
};

namespace Training {
    static double lr = 0.5;



}

namespace Camera {
    static bool partitioning = true;
    static double exposure_time_us = 300.0f;
}

}

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
static std::string dataset_path = "./Datasets/";

struct Batch {
    std::vector<Texture> textures;
    std::vector<int>     labels;
};

std::vector<Batch> Get (
    int n_data_points,
    int batch_size,
    s2_DataTypes dtype=s2_DataTypes::TRAIN,
    int padding = 0
);

std::vector<Batch> Get_Training ();
std::vector<Batch> Get_Validation ();
std::vector<Batch> Get_Test ();
void               Delete (std::vector<Batch> &);

}

namespace Run {

void Setup_Scheduler (
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
    double compute_time_ms;

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
};


Performance Evaluate (
    Scheduler2 &,
    EvalFunctions &,
    Data::Batch &
);

Performance Evaluate (
    Scheduler2 &,
    EvalFunctions &,
    std::vector<Data::Batch> &
);

Performance Inference (
    Scheduler2 &,
    EvalFunctions &,
    Data::Batch &
);

Performance Inference (
    Scheduler2 &,
    EvalFunctions &,
    std::vector<Data::Batch> &
);


void Iterate (Scheduler2 &);

}


}


#endif
