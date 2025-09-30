#include "e23.hpp"

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

namespace e23_global {
    static std::atomic<double> correct = 0;
    static std::atomic<double> total   = 0;
    static std::atomic<bool> acc_norm {false};
    static std::atomic<bool> enable_norm {false};
    static double norms[10] = {
        14.44705867767334, 9.843138694763184, 8.145097732543945, 10.52156925201416, 18.941179275512695, 13.25882339477539, 9.607842445373535, 7.874510765075684, 14.803922653198242, 12.537257194519043
    };
    static double gains     = 1.0;
    torch::Tensor masks;
    torch::Tensor noise_bg;

    int64_t label_counts[10] = {0};
    int64_t label_freq[10] = {0};
    std::atomic<bool> enable_counts {false};

    std::function<void()> clear_label_counts = []()->void {
        for (int i = 0; i < 10; ++i) {
            label_counts[i] = 0;
            label_freq[i] = 0;
        }
    };
    int64_t label;
    int64_t process_count;

    std::vector<int> label_history;
}

struct e23_Batch {
    std::vector<Texture> textures;
    std::vector<int>     labels;
};


struct e23_Validation_Batch {
    Texture texture;
    std::vector<int> labels;
};

struct e23_DataPoints {
    double results[10];
    int    label;
};

static std::vector<e23_DataPoints> data_points;
std::atomic<bool> start_saving_data_points {false};

void e23_SaveDataPoints () {
    // Save it to a file
    std::ofstream ofs("data_points.txt");
    for (const auto& dp : data_points) {
        ofs << dp.label << " ";
        for (const auto& r : dp.results) {
            ofs << r << " ";
        }
        ofs << "\n";
    }
}


std::vector<e23_Batch> Get_Data(int n_data_points, int batch_size, s2_DataTypes dtype=s2_DataTypes::TRAIN) {
    s2_Dataloader data_loader {"./Datasets/"};
    auto data = data_loader.load(dtype, n_data_points);
    std::cout << "INFO: [e23] Loaded data with " << data.len() << " samples.\n";

    std::vector<e23_Batch> batches;
    batches.resize(n_data_points / batch_size);

    for (int i = 0; i < n_data_points; i += batch_size) {
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

class e23_Normal : public Dist {
public:
    e23_Normal () {}

    e23_Normal (torch::Tensor &mu, double std) :
    m_mu(mu), m_std(std) {}

    ~e23_Normal () override {}

    torch::Tensor sample (int n) override {
        torch::NoGradGuard no_grad;
        
        // Broadcast m_mu to match the sample shape
        auto mu_shape = m_mu.sizes();
        auto sample_shape = torch::IntArrayRef({n}).vec();
        sample_shape.insert(sample_shape.end(), mu_shape.begin(), mu_shape.end());


        torch::Tensor eps = torch::randn(sample_shape, m_mu.options());
        return m_mu.unsqueeze(0).expand_as(eps) + m_std * eps;
    }

    torch::Tensor sample_antithetic (int n) {
        torch::NoGradGuard no_grad;
        int m = n / 2;
        
        // Broadcast m_mu to match the sample shape
        auto mu_shape = m_mu.sizes();
        auto sample_shape = torch::IntArrayRef({m}).vec();
        sample_shape.insert(sample_shape.end(), mu_shape.begin(), mu_shape.end());

        torch::Tensor eps = torch::randn(sample_shape, m_mu.options());
        auto x1 = m_mu.unsqueeze(0).expand_as(eps) + m_std * eps;
        auto x2 = m_mu.unsqueeze(0).expand_as(eps) - m_std * eps;
        return torch::cat({x1, x2}, 0);
    }

    torch::Tensor log_prob(torch::Tensor &t) override {
        // Compute log probability of t under Normal(m_mu, m_std)
        auto var = m_std * m_std;
        auto log_scale = std::log(m_std);

        return (
            -((t - m_mu).pow(2)) / (2 * var)
            - log_scale
            - std::log(2 * M_PI) / 2
        );

        //auto log_probs = -0.5 * ((t - m_mu).pow(2) / var + 2 * log_scale + std::log(2 * M_PI));
        //return log_probs;
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

class e23_Model : public s4_Model {
public:

    e23_Model () {} /* Default constructor */

    e23_Model (int64_t Height, int64_t Width, int64_t n) :
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

        std::cout<<"INFO: [e23_Model] Staging model...\n";
        std::cout<<"INFO: [e23_Model] Height: " << Height << ", Width: " << Width << ", Number of Perturbations (samples): " << n << '\n';

        //m_parameter = torch::rand({Height, Width}, torch::kFloat16).to(DEVICE) * 2 * M_PI - M_PI;

        // compute parameter with GSA
        torch::Tensor ones = torch::ones({Height, Width}, torch::kFloat16).to(DEVICE);
        m_parameter = s4_Utils::GSAlgorithm(ones, 50);

        //m_parameter = torch::zeros({Height, Width}).to(DEVICE);
        m_parameter.set_requires_grad(true);
        std::cout<<"INFO: [e23_Model] Set parameters...\n";

        m_dist.set_mu(m_parameter, kappa);
        std::cout<<"INFO: [e23_Model] Created distribution...\n";
    }

    ~e23_Model () override {}

    torch::Tensor sample(int n, bool antithetic=false) {
        torch::NoGradGuard no_grad;
        torch::Tensor x;
        if (antithetic) {
            x = m_dist.sample_antithetic(n);
        }
        else {
            x = m_dist.sample(n);
        }

        // Keep action in [-pi, pi] using where
        x = torch::where(x > M_PI, M_PI, x);
        x = torch::where(x < -M_PI, -M_PI, x);

        // Quantize the actions
        // auto action = q(x, false);

        auto action = x;
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

    Quantize q;
    int64_t m_Height;
    int64_t m_Width;
    int64_t m_n;

    torch::Tensor m_parameter;
    torch::Tensor m_action;

    //VonMises m_dist {};
    e23_Normal m_dist {};

    const double std = 0.05f;
    const double kappa = 1.0f/std;

    std::vector<torch::Tensor> m_action_s;  /* Used for sequential creation. */
};

std::pair<torch::Tensor, bool> e23_ProcessFunction (CaptureData &ts) {
    static int64_t process_count = 0;
    std::cout << "INFO: [e23_ProcessFunction]: Processed: " << process_count + 1 << " so far.\n";

    // Sum to [16]
    /*
    if (ts.label <= 4) {
        ts.label = 0;
    } else {
        ts.label = 1;
    }
    */
    //ts.label = e23_global::label;
    //assert (ts.label == e23_global::label);
    e23_global::label_history.push_back(ts.label);

    auto t = ts.image;
    auto k = ts.full; // full image
    auto l = torch::tensor({ts.label}); // label


    k = k.to(torch::kFloat32);// - e23_global::noise_bg.to(torch::kFloat32);
    // if k is negative, set to zero
    k = torch::clamp(k, 0, 255);
    k = torch::where(k < 30, torch::zeros_like(k), k);

    // if the first one, save the image
    if (process_count % 10'000 == 0) {
        auto kk = k.contiguous().to(torch::kUInt8);
        auto img = s4_Utils::TensorToImage(kk);
        ExportImage(img, "sample.bmp");
        UnloadImage(img);
    }

    // Use the full image (240x480)
    const int region_size = 10;
    const int region_area = region_size * region_size; // 100
    k = k.unsqueeze(0).unsqueeze(0).to(torch::kFloat32); // [1, 1, 240, 480]
    auto sums = (k.unsqueeze(1) * e23_global::masks.to(k.device())).sum({2,3,4}); // [1, 10]

    sums /= 255.0;

    if (e23_global::enable_norm.load(std::memory_order_acquire)) {
        for (int i = 0; i < 10; ++i) {
            double norm = e23_global::norms[i];
            if (norm > 0) {
                sums[0][i] /= norm;
            }
        }
    }

    // Concatenate along the first dimension
    auto predictions = sums;



    // take the argmax
    auto preds = predictions.argmax(1); // [1]

    if (process_count % 20 == 0) {
        if (preds.item<int>() == ts.label) {
            e23_global::correct.fetch_add(1, std::memory_order_release);
        }
        e23_global::total.fetch_add(1, std::memory_order_release);

        if (e23_global::enable_counts.load(std::memory_order_acquire)) {
            e23_global::label_counts[ts.label]++;
            e23_global::label_freq[preds.item<int>()]++;
        }
    }

    auto targets = l.to(torch::kLong).to(t.device()); // [1]

    // score is the cross entropy loss
    
    auto loss_ces = torch::nn::functional::cross_entropy(
        predictions,
        targets,
        torch::nn::functional::CrossEntropyFuncOptions().reduction(torch::kNone)
    );  // [1]
    
    
    // score is MSE
    torch::Tensor one_hot_targets = torch::nn::functional::one_hot(targets, 10).to(torch::kFloat64) * region_area;
    // Compute predictions as probabilities
    //auto prob_predictions = torch::nn::functional::softmax(predictions, 1).unsqueeze(1);
    auto loss_mse = torch::nn::functional::mse_loss(
        predictions,
        one_hot_targets,
        torch::nn::functional::MSELossFuncOptions().reduction(torch::kNone)
    );  // [1]
    loss_mse = loss_mse.sum();

    // The maximum of CES loss is (experimentally) ~3.0
    // The maximum of MSE loss is 10,000

    // Normalize the two losses and apply a weighting to each
    // What we have here is the average of the two losses
    // CES loss tries to quickly get the right answer
    // while MSE loss tries to maximize the power
    double ces_weight = 1.0;
    double mse_weight = 1.0 - ces_weight;
    auto loss = ces_weight * loss_ces + mse_weight * loss_mse;
    
    /*
    std::cout << "Loss: " << loss.item<double>() << '\n';
    std::cout << "One Hot Targets: " << one_hot_targets << '\n';
    std::cout << "Predictions: " << prob_predictions << '\n';
    */

    ++process_count;
    e23_global::process_count = process_count;
    return {-loss, true};
}

int e23 () {
    // Load regions
    e23_global::masks = np2lt::f32("source/Assets/regions.npy"); // [10, 1, 240, 480]
    e23_global::noise_bg = np2lt::u8("source/Assets/noise_bg.npy");

    int  mask_size_ratio = 2; // 8, 4, 2, 1
    bool load_from_checkpoint = false;
    bool connect_to_server = false;
    std::cout << "Loading any checkpoints? [y/n] ";
    std::string response, checkpoint_dir;
    std::cin >> response;
    load_from_checkpoint = (response == "y");
    if (load_from_checkpoint) {
        std::cout << "Enter checkpoint directory: ";
        std::cin >> checkpoint_dir;
    }
    std::cout << "Do you want to save checkpoints during training? [y/n] ";
    std::cin >> response;
    bool save_checkpoints = (response == "y");

    std::cout << "Do you want to connect to remote server? [y/n] ";
    std::cin >> response;
    connect_to_server = (response == "y");

    /* Camera Parameters */
    int Height = 480, Width = 640;
    bool use_partitioning = true;
    bool use_tiles        = true;

    if (use_partitioning) {
        std::cout << "INFO: [e23] Using partitioning mode.\n";
        Height = 480;
        Width  = 640;
    }
    else {
        std::cout << "INFO: [e23] Using non-partitioning mode.\n";
        Height = 480 / 2;
        Width  = 640 / 2;
    }

    Pylon::PylonAutoInitTerm init {};
    Scheduler2 scheduler {};

    e23_Model model {};

    int model_Height  = 800  / mask_size_ratio;
    int model_Width   = 1280 / mask_size_ratio;

#define OPT_ADAM

    model.init(model_Height, model_Width, scheduler.maximum_number_of_frames_in_image);
#if defined(OPT_ADAM)
    double lr = 0.05f;
    torch::optim::Adam opt_m (model.parameters(), torch::optim::AdamOptions(lr));
#else
    double lr = 10.0f;
    torch::optim::SGD opt_m (model.parameters(), torch::optim::SGDOptions(lr));
#endif
    s4_Optimizer opt (opt_m, model);

    HComms *comms = nullptr;
    if (connect_to_server) {
        comms = new HComms {"192.168.193.20", 9001};
    }

    // macbook-pro 192.168.193.20
    // pop-os      192.168.103.204

    PDFunction process_function = [](CaptureData ts)->std::pair<torch::Tensor, bool> {
        return e23_ProcessFunction(ts);
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
    scheduler.EnableLabelQueueing();
    scheduler.EnableBlendMode();    /* For use with DLP/PLM system */

    // Get image data
    int64_t n_training_samples = 1000;
    int64_t n_batch_size       = 20;
    int64_t n_samples          = 32;    // Note actual number of samples is n_samples * 20
    bool    antithetic         = true;
    /*
    if (antithetic) {
        scheduler.EnableAntiTheticSampling();
    }
    */

    auto batches = Get_Data(n_training_samples, n_batch_size, s2_DataTypes::TRAIN);
    scheduler.SetBatchSize(n_batch_size);
    
    int64_t n_validation_samples  = 1000;
    int64_t n_validation_batch_size = 1000;
    auto val_batches = Get_Data(n_validation_samples, n_validation_batch_size, s2_DataTypes::VALID);
    //auto val_batches = batches;

    int64_t step=0;
    int64_t batch_sel=0;

    double  mean_reward = 0.0f;
    double  mean_val_reward = 0.0f;
    double  val_reward = 0.0f;

    std::string cp_dir = "./2025_09_29_002_s16-1";
    // Save dataset information to mean_reward.txt
    {
        std::ofstream ofs(cp_dir + "/a/mean_reward.txt", std::ios::app);
        ofs << "Dataset: MNIST Balanced 1 for training, Balanced 2 for validation\n";
        ofs << "Training samples: " << n_training_samples << ", Batch size: " << n_batch_size << '\n';

        int64_t n_points_per_class[10] = {0};
        for (int i = 0; i < batches.size(); ++i) {
            for (int j = 0; j < batches[i].labels.size(); ++j) {
                n_points_per_class[batches[i].labels[j]]++;
            }
        }
        ofs << "Training samples per class: ";
        for (int i = 0; i < 10; ++i) {
            ofs << n_points_per_class[i] << " ";
        }
        ofs << '\n';

        ofs << "Validation samples: " << n_validation_samples << ", Batch size: " << n_validation_batch_size << '\n';
        for (int i = 0; i < 10; ++i) {
            n_points_per_class[i] = 0;
        }
        for (int i = 0; i < val_batches.size(); ++i) {
            for (int j = 0; j < val_batches[i].labels.size(); ++j) {
                n_points_per_class[val_batches[i].labels[j]]++;
            }
        }
        ofs << "Validation samples per class: ";
        for (int i = 0; i < 10; ++i) {
            ofs << n_points_per_class[i] << " ";
        }
        ofs << '\n';
        ofs << "Model perturbation size: " << model_Height << " x " << model_Width << '\n';
        ofs << "Number of perturbations per image: " << n_samples << '\n';
        ofs << "Antithetic sampling: " << (antithetic ? "Enabled" : "Disabled") << '\n';
        ofs << "Mask size ratio: " << mask_size_ratio << '\n';
        ofs << "Standard Deviation of action distribution: " << model.std << '\n';
        ofs << "Kappa (1/std): " << model.kappa << '\n';
        ofs << "Learning Rate: " << lr << '\n';
#if defined(OPT_ADAM)
        ofs << "Optimizer: Adam\n";
#else   
        ofs << "Optimizer: SGD\n";
#endif
        ofs << "Samples per image: " << scheduler.maximum_number_of_frames_in_image * n_samples << "\n";
        ofs << "Dataset 1 size: " << n_training_samples << ", Dataset 2 size: " << n_validation_samples << '\n';
        ofs << "Dataset 1 batch size: " << n_batch_size << ", Dataset 2 batch size: " << n_validation_batch_size << '\n';
        ofs << "----------------------------------------\n";
    }

    // Save full dataset information to log
    {
        std::ofstream ofs(cp_dir + "/a/log.txt", std::ios::app);
        ofs << "Dataset: MNIST Balanced 1 for training, Balanced 2 for validation\n";
        ofs << "Balanced 1\n";
        for (int i = 0; i < batches.size(); ++i) {
            ofs << "Batch " << i << " labels: ";
            for (int j = 0; j < batches[i].labels.size(); ++j) {
                ofs << batches[i].labels[j] << " ";
            }
            ofs << "\n";
        }
        ofs << "Balanced 2\n";
        for (int i = 0; i < val_batches.size(); ++i) {
            ofs << "Batch " << i << " labels: ";
            for (int j = 0; j < val_batches[i].labels.size(); ++j) {
                ofs << val_batches[i].labels[j] << " ";
            }
            ofs << "\n";
        }
        ofs << "----------------------------------------\n";
    }

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

    /////////////////////////////////////////////////////
    // Normalization                                   //
    /////////////////////////////////////////////////////

    /*
    e23_global::acc_norm.store(true, std::memory_order_release);
    std::cout << "INFO: [e23] Computing normalization constants...\n";
    for (int i = 0; i < 10; ++i) {
        torch::Tensor action = model.sample(scheduler.maximum_number_of_frames_in_image);
        scheduler.SetTextureFromTensorTiled(action);
        Iterate();
    }
    model.squash();
    scheduler.Dump();
    e23_global::acc_norm.store(false, std::memory_order_release);
    */
    e23_global::enable_norm.store(false, std::memory_order_release);



    int64_t iter = 0;
    const int64_t val_interval = batches.size();
    int validation_accuracy = 0;
    int training_accuracy = 0;

    int64_t training_label_counts[10] = {0};
    int64_t training_label_freq[10] = {0};
    int64_t validation_label_counts[10] = {0};
    int64_t validation_label_freq[10] = {0};
    int64_t n_sent = 0;
    int64_t n_processed = 0;
    int64_t n_previous_processed = 0;
    while (!WindowShouldClose()) {

        /////////////////////////////////////////////////////
        // Training                                        //
        /////////////////////////////////////////////////////

        // Clear label history
        e23_global::label_history.clear();

        std::vector<int> labels_used;
        int64_t time_0 = Utils::GetCurrentTime_us();
        e23_global::clear_label_counts();
        e23_global::enable_counts.store(true, std::memory_order_release);
        for (int i =0; i < n_samples; ++i) {
            torch::Tensor action = model.sample(scheduler.maximum_number_of_frames_in_image, antithetic);
            scheduler.SetTextureFromTensorTiled(action);
            //action = torch::fft::fftshift(action, {-2, -1});

            if (i == 1) {
                training_accuracy = 1000 * e23_global::correct.load(std::memory_order_acquire) / e23_global::total.load(std::memory_order_acquire);
            }

            for (int j = 0; j < n_batch_size; ++j) {
                e23_global::label = batches[batch_sel].labels[j];
                scheduler.SetLabel(batches[batch_sel].labels[j], 20);
                scheduler.SetSubTextures(batches[batch_sel].textures[j], 0);
                Iterate();
                ++step;
            }
        }

        //TakeScreenshot("train_screen.png");
        std::cout << "INFO: [e23] Training step " << step << " completed...\n";
        scheduler.SetVSYNC_Marker();
        scheduler.WaitVSYNC_Diff(2);

        model.squash();
        n_sent = scheduler.GetNumberOfFramesSent();
        auto reward = scheduler.Update();
        //scheduler.Dump();
        int64_t time_1 = Utils::GetCurrentTime_us();

        int64_t delta = time_1 - time_0;
        //training_accuracy = 1000 * e23_global::correct.load(std::memory_order_acquire) / e23_global::total.load(std::memory_order_acquire);

        // Store into training_label_counts and training_label_freq
        e23_global::enable_counts.store(false, std::memory_order_release);
        for (int i = 0; i < 10; ++i) {
            training_label_counts[i] += e23_global::label_counts[i];
            training_label_freq[i]   += e23_global::label_freq[i];
        }
        e23_global::clear_label_counts();


        /////////////////////////////////////////////////////
        // Validation                                      //
        /////////////////////////////////////////////////////
        
        e23_global::correct.store(0, std::memory_order_release);
        e23_global::total.store(0, std::memory_order_release);
        if (iter % val_interval == (val_interval - 1)) {
            // Get std from model
            e23_global::enable_counts.store(true, std::memory_order_release);
            double std = model.m_dist.get_std();
            model.m_dist.set_std(0.0f);

            std::cout << "INFO: [e23] Running Validation...\n";
            torch::Tensor action = model.sample(scheduler.maximum_number_of_frames_in_image);
            //action = torch::fft::fftshift(action, {-2, -1});
            scheduler.SetTextureFromTensorTiled(action);

            for (int i = 0; i < val_batches.size(); ++i) {
                for (int j = 0; j < n_validation_batch_size; ++j) {
                    e23_global::label = val_batches[i].labels[j];
                    scheduler.SetLabel(val_batches[i].labels[j], 20);
                    scheduler.SetSubTextures(val_batches[i].textures[j], 0);
                    Iterate();
                }
            }
            // set the std back
            model.squash();
            model.m_dist.set_std(std);
            val_reward = scheduler.Loss();
            validation_accuracy = 1000 * e23_global::correct.load(std::memory_order_acquire) / e23_global::total.load(std::memory_order_acquire);


            start_saving_data_points.store(false, std::memory_order_release);
        }
        e23_global::enable_counts.store(false, std::memory_order_release);
        n_processed = e23_global::process_count;

        // Copy
        for (int i = 0; i < 10; ++i) {
            validation_label_counts[i] = e23_global::label_counts[i];
            validation_label_freq[i]   = e23_global::label_freq[i];
        }
        e23_global::clear_label_counts();

        
        /////////////////////////////////////////////////////
        // Networking                                      //
        /////////////////////////////////////////////////////

        // Combine the two scores and transmit it
        // we can reconstruct the training accuracy and validation accuracy
        double combined_accuracy =  ((static_cast<int>(-reward * 1000)) << 20) | (training_accuracy << 10) | validation_accuracy;
        std::cout << "INFO: [e23] Reward: " << reward << '\n';

        // To reconstruct it, we know that
        // we can extract the training_accuracy by shifting right by 10 bits
        // and validation_accuracy is the lower 10 bits

        // Transmit the data to remote server
        if (comms) {
            HCommsDataPacket_Outbound packet;
            packet.reward = combined_accuracy; // = reward;
            packet.step   = step;
            packet.image  = scheduler.GetSampleImage().contiguous().to(torch::kUInt8);

            // Send the packet
            comms->Transmit(packet);
        }
        /*
        auto k = scheduler.GetSampleImage().contiguous().to(torch::kUInt8);
        auto img = s4_Utils::TensorToImage(k);
        ExportImage(img, "sample.png");
        UnloadImage(img);
        */
        scheduler.DisposeSampleImages();

        e23_global::correct.store(0, std::memory_order_release);
        e23_global::total.store(0, std::memory_order_release);
        ++iter;

        /////////////////////////////////////////////////////
        // Saving checkpoints                              //
        /////////////////////////////////////////////////////
        if (save_checkpoints) {
            Scheduler2_CheckPoint cp;
            cp.batch_id = batch_sel;
            cp.training_accuracy = training_accuracy;
            cp.validation_accuracy = validation_accuracy;
            cp.val_reward = mean_val_reward;
            cp.phase = model.get_parameters();
            cp.kappa = 1/model.m_dist.get_std();
            cp.step = step;
            cp.dataset_path = "./Datasets";
            cp.checkpoint_dir = cp_dir;
            cp.checkpoint_name = "";
            cp.reward = reward;
            scheduler.SaveCheckpoint(cp);
        }

        batch_sel = (batch_sel + 1) % batches.size();

        // Logging
        // Get the number of samples sent
        /*
        {
            std::ofstream log_file(cp_dir + "/log.txt", std::ios::app);
            log_file << "Iteration: " << iter << ", Step: " << step << "\n";
            log_file << "Number Of Frames Sent: " << n_sent << '\n';
            log_file << "Process Count: " << n_processed - n_previous_processed << '\n';

            log_file << "Expected Process Count: " << (n_samples * scheduler.maximum_number_of_frames_in_image * n_batch_size) << '\n';

            // Get Labels used
            log_file << "Labels used in this iteration:\n";
            // Print every maximum_number_of_frames_in_image labels
            int j = 0;
            for (int i = 0; i < e23_global::label_history.size(); ++i) {
                if (i % (scheduler.maximum_number_of_frames_in_image * n_batch_size) == 0) {
                    log_file << "Sample: " << j << '\n';
                    ++j;
                }
                log_file << e23_global::label_history[i] << " ";
                if ((i + 1) % scheduler.maximum_number_of_frames_in_image == 0) {
                    log_file << '\n';
                }
            }
            log_file << "\n";
            n_previous_processed = n_processed;
        }
        */

        mean_reward += reward;
        mean_val_reward = val_reward;

        if (batch_sel == 0) {
            mean_reward /= static_cast<double>(batches.size());
            //mean_val_reward /= static_cast<double>(val_batches.size());
            // Save mean reward to file (append)
            std::ofstream ofs(cp_dir + "/a/mean_reward.txt", std::ios::app);
            ofs << "Epoch: " << iter / batches.size() << '\n';
            ofs << "Training Loss: "  << mean_reward << ", Validation Loss: " << mean_val_reward;
            ofs << ", Training Acc: " << training_accuracy / 10.0f << "%, Validation Acc: " << validation_accuracy / 10.0f << "%\n";

            ofs << "Training Label Counts:\n";
            for (int i = 0; i < 10; ++i) {
                ofs << training_label_counts[i] << " ";
            }
            ofs << '\n';

            ofs << "Training Label Freq:\n";
            for (int i = 0; i < 10; ++i) {
                ofs << training_label_freq[i] << " ";
            }
            ofs << '\n';

            ofs << "Validation Label Counts:\n";
            for (int i = 0; i < 10; ++i) {
                ofs << validation_label_counts[i] << " ";
            }
            ofs << '\n';

            ofs << "Validation Label Freq:\n";
            for (int i = 0; i < 10; ++i) {
                ofs << validation_label_freq[i] << " ";
            }
            ofs << "\n---\n";

            mean_reward = 0.0f;
            mean_val_reward = 0.0f;

            // Clear both
            for (int i = 0; i < 10; ++i) {
                training_label_counts[i] = 0;
                training_label_freq[i] = 0;
                validation_label_counts[i] = 0;
                validation_label_freq[i] = 0;
            }
        }
    }

    scheduler.StopThreads();
    scheduler.StopCamera();
    scheduler.StopWindow();

    for (auto &b : batches) {
        for (auto &t : b.textures) {
            UnloadTexture(t);
        }
    }

    if (comms) delete comms;
    return 0;
}