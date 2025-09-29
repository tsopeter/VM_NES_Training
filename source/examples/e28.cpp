#include "e28.hpp"

#include <torch/torch.h>

#include "../s4/model.hpp"
#include "../s4/optimizer.hpp"
#include "../s5/scheduler2.hpp"
#include "../s5/hcomms.hpp"
#include "../utils/utils.hpp"
#include "../s4/utils.hpp"
#include "../s2/dataloader.hpp"
#include "../s2/von_mises.hpp"
#include <fstream>
#include <ostream>

namespace e28_global {
    static std::atomic<double> correct = 0;
    static std::atomic<double> total   = 0;
    static std::atomic<bool> acc_norm {false};
    static std::atomic<bool> enable_norm {false};
    static double norms[10] = {0};
    static double gains     = 1.0;
    static torch::Tensor noise_bg = torch::tensor(
        std::vector<double>{837, 765, 714, 788, 353, 342, 314, 371, 365, 354,
        0, 0, 0, 0, 0, 0},
        torch::dtype(torch::kFloat64)
    ).reshape({16});
    std::vector<torch::Tensor> images;
    std::vector<int> labels;
    std::vector<int> preds;
}

struct e28_Batch {
    std::vector<Texture> textures;
    std::vector<int>     labels;
};


struct e28_Validation_Batch {
    Texture texture;
    std::vector<int> labels;
};

struct e28_DataPoints {
    double results[10];
    int    label;
};

static std::vector<e28_DataPoints> e28_e28_data_points;
std::atomic<bool> e28_start_saving_e28_e28_data_points {false};

void e28_SaveDataPoints () {
    // Save it to a file
    std::ofstream ofs("e28_e28_data_points.txt");
    for (const auto& dp : e28_e28_data_points) {
        ofs << dp.label << " ";
        for (const auto& r : dp.results) {
            ofs << r << " ";
        }
        ofs << "\n";
    }
}


std::vector<e28_Batch> e28_Get_Data(int n_e28_e28_data_points, int batch_size, s2_DataTypes dtype=s2_DataTypes::TRAIN) {
    s2_Dataloader data_loader {"./Datasets/"};
    auto data = data_loader.load(dtype, n_e28_e28_data_points);
    std::cout << "INFO: [e28] Loaded data with " << data.len() << " samples.\n";

    std::vector<e28_Batch> batches;
    batches.resize(n_e28_e28_data_points / batch_size);

    for (int i = 0; i < n_e28_e28_data_points; i += batch_size) {
        for (int j = 0; j < batch_size; ++j) {
            auto [d, l] = data[i + j];
            // Process the data

            auto li = l.item<int>();

            d = 255.0 - d;
            Image di = s4_Utils::TensorToImage(d);
            Texture ti = LoadTextureFromImage(di);
            SetTextureFilter(ti, TEXTURE_FILTER_BILINEAR);
            UnloadImage(di);

            batches[i / batch_size].textures.push_back(ti);
            batches[i / batch_size].labels.push_back(li);
        }

    }

    return batches;
}


class e28_Normal : public Dist {
public:
    e28_Normal () {}

    e28_Normal (torch::Tensor &mu, double std) :
    m_mu(mu), m_std(std) {}

    ~e28_Normal () override {}

    torch::Tensor sample (int n) override {
        torch::NoGradGuard no_grad;
        
        // Broadcast m_mu to match the sample shape
        auto mu_shape = m_mu.sizes();
        auto sample_shape = torch::IntArrayRef({n}).vec();
        sample_shape.insert(sample_shape.end(), mu_shape.begin(), mu_shape.end());


        torch::Tensor eps = torch::randn(sample_shape, m_mu.options());
        return m_mu.unsqueeze(0).expand_as(eps) + m_std * eps;
    }

    torch::Tensor log_prob(torch::Tensor &t) override {
        // Compute log probability of t under Normal(m_mu, m_std)
        auto var = m_std * m_std;
        auto log_scale = std::log(m_std);
        auto log_probs = -0.5 * ((t - m_mu).pow(2) / var + 2 * log_scale + std::log(2 * M_PI));
        return log_probs;
    }

    void set_mu (torch::Tensor &mu, double kappa) {
        m_mu = mu;
        m_std = 1.0f/kappa;
    }

    void set_std (double std) {
        m_std = std;
    }

    double get_std () {
        return m_std;
    }

    torch::Tensor m_mu;
    double m_std;


};

class e28_Model : public s4_Model {
public:

    e28_Model () {} /* Default constructor */

    e28_Model (int64_t Height, int64_t Width, int64_t n) :
    m_Height(Height), m_Width(Width), m_n(n) {
        init (Height, Width, n);
    }

    void set_params (torch::Tensor t, double kappa) {
        m_parameter = t;
        m_parameter.set_requires_grad(true);
        m_dist.set_mu(m_parameter, kappa);
    }

    void init (int64_t Height, int64_t Width, int64_t n) {
        m_Height = Height;
        m_Width  = Width;
        m_n      = n;

        std::cout<<"INFO: [e28_Model] Staging model...\n";
        std::cout<<"INFO: [e28_Model] Height: " << Height << ", Width: " << Width << ", Number of Perturbations (samples): " << n << '\n';

        //m_parameter = torch::rand({Height, Width}, torch::kFloat16).to(DEVICE) * 2 * M_PI - M_PI;

        // compute parameter with GSA
        torch::Tensor ones = torch::ones({Height, Width}, torch::kFloat16).to(DEVICE);
        m_parameter = s4_Utils::GSAlgorithm(ones, 50);

        //m_parameter = torch::zeros({Height, Width}).to(DEVICE);
        m_parameter.set_requires_grad(true);
        std::cout<<"INFO: [e28_Model] Set parameters...\n";

        m_dist.set_mu(m_parameter, kappa);
        std::cout<<"INFO: [e28_Model] Created distribution...\n";
    }

    ~e28_Model () override {}

    torch::Tensor sample(int n) {
        torch::NoGradGuard no_grad;
        torch::Tensor action = m_dist.sample(n);

        m_action_s.push_back(action);
        return action;
    }

    void squash () {
        if (m_action_s.empty()) return;

        m_action = torch::cat(m_action_s, 0);  // [k * n, H, W]
        m_action_s.clear();
    }

    torch::Tensor sample () {
        torch::NoGradGuard no_grad;
        m_action = m_dist.sample(m_n);
        return m_action;
    }

    torch::Tensor logp_action () override {
        return m_dist.log_prob(m_action);
    }

    std::vector<torch::Tensor> parameters () {
        return {m_parameter};
    }

    torch::Tensor &get_parameters () {
        return m_parameter;
    }

    int64_t N_samples () const override {
        return m_action.size(0);
    }

    int64_t m_Height;
    int64_t m_Width;
    int64_t m_n;

    torch::Tensor m_parameter;
    torch::Tensor m_action;

    //VonMises m_dist {};
    e28_Normal m_dist {};

    const double std = 0.05 * (2 * M_PI); // 10% of the range
    const double kappa = 1.0f/std;

    std::vector<torch::Tensor> m_action_s;  /* Used for sequential creation. */
};


std::pair<torch::Tensor, bool> e28_ProcessFunction (CaptureData &ts) {
    static int64_t process_count = 0;
    std::cout << "INFO: [e28_ProcessFunction]: Processed: " << process_count + 1 << " so far.\n";

    // Sum to [16]
    /*
    if (ts.label <= 4) {
        ts.label = 0;
    } else {
        ts.label = 1;
    }
    */

    auto t = ts.image;
    auto k = ts.full; // full image
    auto l = torch::tensor({ts.label}); // label


    //
    // Regions are organized as the following zones
    // 0 1 2 3
    // 4 5 6 7
    // 8 9 A B
    // C D E F

    // Let the zones we actually care about be
    // 5   7
    // 9 A B

    // Divide the 4 zones into 10 labels as such
    // 5 -> [0, 1]
    // 7 -> [2, 3]
    // 9 -> [4, 5]
    // A -> [6, 7]
    // B -> [8, 9]

    // Each zone is 60x60
    // so we can divide each zone into 4 areas of 15x15
    // where we have 15px padding between areas
    /*
    auto lb_0 = t[5].slice(0, 0,  15).slice(1,  0, 15);   // get label 0
    auto lb_1 = t[5].slice(0, 0,  15).slice(1, 45, 60);  // get label 1
    auto lb_2 = t[5].slice(0, 45, 60).slice(1,  0, 15);  // get label 2
    auto lb_3 = t[5].slice(0, 45, 60).slice(1, 45, 60); // get label 3

    auto lb_4 = t[7].slice(0, 0, 15).slice(1, 0, 15);   // get label 4
    auto lb_5 = t[7].slice(0, 45, 60).slice(1, 0, 15);  // get label 5
    auto lb_6 = t[9].slice(0, 45, 60).slice(1, 0, 15);  // get label 6
    auto lb_7 = t[9].slice(0, 45, 60).slice(1, 45, 60); // get label 7

    auto lb_8 = t[11].slice(0, 0, 15).slice(1, 0, 15);   // get label 8
    auto lb_9 = t[11].slice(0, 45, 60).slice(1, 0, 15); // get label 9
    */

    // 10x10 regions
    const int region_size = 10;
    const int region_area = region_size * region_size;
    auto lb_0 = t[5].slice(0, 0,  region_size).slice(1,  0, region_size);   // get label 0
    auto lb_1 = t[5].slice(0, 0,  region_size).slice(1, 60-region_size, 60);  // get label 1
    auto lb_2 = t[5].slice(0, 60-region_size, 60).slice(1,  0, region_size);  // get label 2
    auto lb_3 = t[5].slice(0, 60-region_size, 60).slice(1, 60-region_size, 60); // get label 3

    auto lb_4 = t[7].slice(0, 0, region_size).slice(1, 0, region_size);   // get label 4
    auto lb_5 = t[7].slice(0, 60-region_size, 60).slice(1, 0, region_size);  // get label 5
    auto lb_6 = t[9].slice(0, 60-region_size, 60).slice(1, 0, region_size);  // get label 6
    auto lb_7 = t[9].slice(0, 60-region_size, 60).slice(1, 60-region_size, 60); // get label 7

    auto lb_8 = t[11].slice(0, 0, region_size).slice(1, 0, region_size);   // get label 8
    auto lb_9 = t[11].slice(0, 60-region_size, 60).slice(1, 0, region_size); // get label 9

    // get label 0 and 1

    // Cat together
    t = torch::stack({lb_0, lb_1, lb_2, lb_3, lb_4, lb_5, lb_6, lb_7, lb_8, lb_9}, 0); // [10, 30, 60]

    auto sums = t.sum({1,2}).to(torch::kFloat64);   // [16]

    //sums = sums - e28_global::noise_bg;

    // Each area is 60x60 = 3600 pixels
    sums /= 255.0;

    auto ksum = k.sum() - sums.sum();

    std::vector<torch::Tensor> t_vec;

    if (e28_global::enable_norm)
        for (int i = 0; i < 10; ++i) {
            t_vec.push_back(e28_global::gains * e28_global::norms[i] * sums[i].unsqueeze(0).to(torch::kFloat64));
        }
    else
        for (int i = 0; i < 10; ++i) {
            t_vec.push_back(e28_global::gains * sums[i].unsqueeze(0).to(torch::kFloat64));
        }

    if (e28_global::acc_norm) {
        // Sum up the values
        for (int i = 0; i < 10; ++i) {
            e28_global::norms[i] += t_vec[i].squeeze().item<double>();
        }
    }

    // Concatenate along the first dimension
    auto predictions = torch::stack(t_vec, 1);  // [1, 10]



    // take the argmax
    auto preds = predictions.argmax(1); // [1]

    if (process_count % 20 == 0) {
        std::cout << "Predicted: " << preds.item<int>() << ", Actual: " << ts.label << '\n';
        if (preds.item<int>() == ts.label) {
            e28_global::correct.fetch_add(1, std::memory_order_release);
        }
        e28_global::total.fetch_add(1, std::memory_order_release);
        e28_global::preds.push_back(preds.item<int>());
        e28_global::labels.push_back(ts.label);

        // Save torch as image
        e28_global::images.push_back(k);
    }

    auto targets = l.to(torch::kLong).to(t.device()); // [1]

    // score is the cross entropy loss
    /*
    auto loss = torch::nn::functional::cross_entropy(
        predictions,
        targets,
        torch::nn::functional::CrossEntropyFuncOptions().reduction(torch::kNone)
    );  // [1]
    */
    
    // score is MSE
    torch::Tensor one_hot_targets = torch::nn::functional::one_hot(targets, 10).to(torch::kFloat64) * region_area;
    // Compute predictions as probabilities
    //auto prob_predictions = torch::nn::functional::softmax(predictions, 1).unsqueeze(1);
    auto loss = torch::nn::functional::mse_loss(
        predictions,
        one_hot_targets
    );  // [1]
    
    /*
    std::cout << "Loss: " << loss.item<double>() << '\n';
    std::cout << "One Hot Targets: " << one_hot_targets << '\n';
    std::cout << "Predictions: " << prob_predictions << '\n';
    */

    ++process_count;
    return {-loss, true};
}

int e28 () {
    std::string checkpoint_dir;
    int  mask_size_ratio = 2; // 8, 4, 2, 1
    bool load_from_checkpoint = false;
    load_from_checkpoint = true;
    if (load_from_checkpoint) {
        std::cout << "Enter checkpoint directory: ";
        std::cin >> checkpoint_dir;
    }
    bool save_checkpoints = false;

    /* Camera Parameters */
    int Height = 480, Width = 640;
    bool use_partitioning = true;
    bool use_tiles        = true;

    if (use_partitioning) {
        std::cout << "INFO: [e28] Using partitioning mode.\n";
        Height = 480;
        Width  = 640;
    }
    else {
        std::cout << "INFO: [e28] Using non-partitioning mode.\n";
        Height = 480 / 2;
        Width  = 640 / 2;
    }

    Pylon::PylonAutoInitTerm init {};
    Scheduler2 scheduler {};

    e28_Model model {};

    int model_Height  = 800  / mask_size_ratio;
    int model_Width   = 1280 / mask_size_ratio;

    model.init(model_Height, model_Width, scheduler.maximum_number_of_frames_in_image);
    torch::optim::Adam opt_m (model.parameters(), torch::optim::AdamOptions(0.05));
    //torch::optim::SGD opt_m (model.parameters(), torch::optim::SGDOptions(100.0f));
    s4_Optimizer opt (opt_m, model);

    // macbook-pro 192.168.193.20
    // pop-os      192.168.103.204

    PDFunction process_function = [](CaptureData ts)->std::pair<torch::Tensor, bool> {
        return e28_ProcessFunction(ts);
    };

    scheduler.Start(
        /* Windowing */
        0,
        1600,
        2560,
        FULLSCREEN,
        NO_TARGET_FPS,
        30,

        /* Camera */
        Height,
        Width,
        59.0f, /* Exposure Time */
        1,  /* Binning Horizontal */
        1,  /* Binning Vertical */
        3,  /* Line Trigger */
        use_partitioning,  /* Use Zones */
        4,  /* Number of Zones */
        60, /* Zone Size */
        true, /* Use Centering */
        0, /* Offset X */
        0, /* Offset Y */
        8, /* Pixel Format: 8 for Mono8, 10 for Mono10 */

        /* PEncoder properties */
        /* These actually determine the size of the texture */
        model_Height * 2,
        model_Width  * 2,

        /* Optimizer */
        &opt,

        /* Processing function */
        process_function
    );
    scheduler.EnableSampleImageCapture();
    scheduler.SetRewardDevice(DEVICE);

    // Get image data
    int64_t n_training_samples = 100;
    int64_t n_batch_size       = 20;
    int64_t n_samples          = 32;    // Note actual number of samples is n_samples * 20

    auto batches = e28_Get_Data(n_training_samples, n_batch_size, s2_DataTypes::TRAIN);
    scheduler.SetBatchSize(n_batch_size);

    
    int64_t n_validation_samples  = 100;
    int64_t n_validation_batch_size = 20;
    auto val_batches = e28_Get_Data(n_validation_samples, n_validation_batch_size, s2_DataTypes::TRAIN);
    //auto val_batches = batches;

    int64_t step=0;
    int64_t batch_sel=0;

    double  mean_reward = 0.0f;
    std::vector<int> labels;

    auto Iterate = [&scheduler]->void {
        for (int i = 0; i < 4; ++i) {
            //scheduler.DrawTextureToScreenTiled();
            scheduler.DrawTextureToScreenCentered();

            scheduler.SetVSYNC_Marker();
            scheduler.WaitVSYNC_Diff(1);
        }
        scheduler.ReadFromCamera();
    };


    /////////////////////////////////////////////////////
    // Checkpoint                                      //
    /////////////////////////////////////////////////////
    if (load_from_checkpoint) {
        auto cp = scheduler.LoadCheckpoint(checkpoint_dir);

        // Set model
        model.set_params(
            cp.phase,
            cp.kappa
        );

        // set the batch_sel
        batch_sel = (cp.batch_id + 1) % batches.size();

        // set the step
        step = cp.step;
    }


    int64_t iter = 0;
    const int64_t val_interval = 0; // every 10 iterations
    int validation_accuracy = 0;
    int training_accuracy = 0;

    while (!WindowShouldClose()) {

        /////////////////////////////////////////////////////
        // Validation                                      //
        /////////////////////////////////////////////////////
        
        e28_global::correct.store(0, std::memory_order_release);
        e28_global::total.store(0, std::memory_order_release);
        model.m_dist.set_std(0.0f);

        std::cout << "INFO: [e28] Running Validation...\n";
        torch::Tensor action = model.sample(scheduler.maximum_number_of_frames_in_image);
        scheduler.SetTextureFromTensorTiled(action);


        for (int i = 0; i < val_batches.size(); ++i) {
            for (int j = 0; j < n_validation_batch_size; ++j) {
                scheduler.SetSubTextures(val_batches[i].textures[j], 0);
                scheduler.SetLabel(val_batches[i].labels[j]);
                Iterate();
                labels.push_back(val_batches[i].labels[j]);
            }
        }

        // set the std back
        model.squash();
        scheduler.Dump();
        validation_accuracy = 1000 * e28_global::correct.load(std::memory_order_acquire) / e28_global::total.load(std::memory_order_acquire);


        e28_start_saving_e28_e28_data_points.store(false, std::memory_order_release);
 
        break;
    }

    // Wait
    std::this_thread::sleep_for(std::chrono::seconds(1));

    scheduler.StopThreads();
    scheduler.StopCamera();
    scheduler.StopWindow();

    for (auto &b : batches) {
        for (auto &t : b.textures) {
            UnloadTexture(t);
        }
    }

    // Save to folder called export, where
    // each image is saved as index.png
    for (int i = 0; i < e28_global::images.size(); ++i) {
        auto img = e28_global::images[i];
        Image im = s4_Utils::TensorToImage(img);
        std::string filename = "./export/" + std::to_string(i) + "_" + std::to_string(e28_global::labels[i]) + "_p" + std::to_string(e28_global::preds[i]) + ".png";
        ExportImage(im, filename.c_str());
        UnloadImage(im);
    }

    // Print out validation accuracy
    std::cout << "INFO: [e28] Total samples: " << e28_global::total.load(std::memory_order_acquire) << '\n';
    std::cout << "Total Labels: " << e28_global::labels.size() << '\n';
    std::cout << "Total Predictions: " << e28_global::preds.size() << '\n';
    std::cout << "INFO: [e28] Validation accuracy: " << validation_accuracy / 10.0f << "%\n";

    return 0;
}