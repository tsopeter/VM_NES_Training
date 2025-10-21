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
#include "../s5/distributions.hpp"

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

    bool antithetic = false;

    std::vector<int> label_history;
    Distributions::Definition* dist = nullptr;
    double alpha = 1e-1;
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


std::vector<e23_Batch> Get_Data(int n_data_points, int batch_size, s2_DataTypes dtype=s2_DataTypes::TRAIN, int padding=0) {
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

            // Pad the image if padding > 0
            if (padding > 0) {
                d = torch::nn::functional::pad(
                    d.unsqueeze(0).unsqueeze(0), // [1, 1, H, W]
                    torch::nn::functional::PadFuncOptions({padding, padding, padding, padding}).mode(torch::kConstant).value(255)
                ).squeeze(); // [H + 2*padding, W + 2*padding]
            }

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

class e23_Model : public s4_Model {
public:

    e23_Model () {} /* Default constructor */

    e23_Model (int64_t Height, int64_t Width, int64_t n) :
    m_Height(Height), m_Width(Width), m_n(n) {
        init (Height, Width, n);
    }

    void init (int64_t Height, int64_t Width, int64_t n) {
#define DIST_CATEGORICAL
#undef  DIST_BINARY
#undef  DIST_NORMAL

        m_n      = n;
#if defined(DIST_BINARY)
        m_Height  = 2*Height; //2*Height for binary or Height for categorical/normal
        m_Width   = 2*Width; // 2*Width  for binary or Width for categorical/normal
#elif defined(DIST_CATEGORICAL) || defined(DIST_NORMAL)
        m_Height  = Height;
        m_Width   = Width;
#endif
#if defined(DIST_CATEGORICAL)
        n_classes = 16;
#elif defined(DIST_BINARY)
        n_classes = 2;
#endif

#if defined(DIST_CATEGORICAL) || defined(DIST_BINARY)
        m_parameter = torch::zeros({m_Height,m_Width,n_classes}, torch::kFloat32).to(DEVICE);
#elif defined(DIST_NORMAL)
        m_parameter = torch::zeros({m_Height,m_Width}, torch::kFloat32).to(DEVICE) * 2 * M_PI - M_PI;
#endif
        m_parameter.set_requires_grad(true);

        std::cout<<"INFO: [e23_Model] Staging model...\n";
        std::cout<<"INFO: [e23_Model] Height: " << m_Height << ", Width: " << m_Width << ", Number of Perturbations (samples): " << n << '\n';

#if defined(DIST_NORMAL)
        m_dist = new Distributions::Normal(m_parameter, std);
#elif defined(DIST_CATEGORICAL)
        m_dist = new Distributions::Categorical(m_parameter);
#elif defined(DIST_BINARY)
        m_dist = new Distributions::Binary(m_parameter);
#endif
    }

    ~e23_Model () override {
        delete m_dist;
    }

    torch::Tensor sample(int n, bool antithetic=false) {
        torch::NoGradGuard no_grad;

        auto action = m_dist->sample(n); // [n, H, W]

        // Store into m_action_s
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
        m_action = m_dist->sample(m_n);
        return m_action;
    }

    torch::Tensor logp_action () override {
        return m_dist->log_prob(m_action);
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
    int64_t n_classes;

    double kappa = 0.0f; // May be unused depending on model
    double std   = 0.1f; // May be unused depending on model

    torch::Tensor m_parameter;
    torch::Tensor m_action;

    Distributions::Definition *m_dist = nullptr;

    std::vector<torch::Tensor> m_action_s;  /* Used for sequential creation. */
};

std::pair<torch::Tensor, bool> e23_ProcessFunction (CaptureData &ts) {
    static int64_t process_count = 0;
    std::cout << "INFO: [e23_ProcessFunction]: Processed: " << process_count + 1 << " so far.\n";


    //ts.label = e23_global::label;
    //assert (ts.label == e23_global::label);
    e23_global::label_history.push_back(ts.label);

    auto t = ts.image;
    auto k = ts.full; // full image
    auto l = torch::tensor({ts.label}); // label


    k = k.to(torch::kFloat32);// - e23_global::noise_bg.to(torch::kFloat32);
    // if k is negative, set to zero
    k = torch::clamp(k, 0, 255);

    const double min_limit = 20;
    k = torch::where(k < min_limit, torch::zeros_like(k), k);

    // map from 10-255 to 0-255
    k = (k - min_limit) * (255.0 / (255.0 - min_limit));
    k = torch::clamp(k, 0, 255).to(torch::kFloat64) / 255.0f;

    // Use the full image (240x480)
    const int region_size = 10;
    const int region_area = region_size * region_size; // 100
    auto qq = k.unsqueeze(0).unsqueeze(0); // [1, 1, 240, 480]
    auto sums = (qq.unsqueeze(1) * e23_global::masks.to(k.device())).sum({2,3,4}); // [1, 10]

    // if the first one, save the image
    if (process_count % 10'000 == 0) {
        auto kk = k * 255.0f;
        kk = kk.contiguous().to(torch::kUInt8);
        auto img = s4_Utils::TensorToImage(kk);
        ExportImage(img, "sample.bmp");
        UnloadImage(img);

        // Save sums to a file called sample.bmp.sums.txt
        std::ofstream ofs("sample.bmp.sums.txt");
        ofs << sums;
        ofs.close();
    }

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
    double ces_weight = 1.0f;
    double mse_weight = 1.0 - ces_weight;
    auto loss = ces_weight * loss_ces + mse_weight * loss_mse;
    
    /*
    std::cout << "Loss: " << loss.item<double>() << '\n';
    std::cout << "One Hot Targets: " << one_hot_targets << '\n';
    std::cout << "Predictions: " << prob_predictions << '\n';
    */

    ++process_count;
    e23_global::process_count = process_count;
    auto entropy = e23_global::dist->entropy_no_grad();
    loss = -loss + e23_global::alpha * entropy.mean().to(loss.device());
    return {loss, true};
}

int e23 () {
    // Load regions
    e23_global::masks = np2lt::f32("source/Assets/regions2.npy"); // [10, 1, 240, 480]
    e23_global::noise_bg = np2lt::u8("source/Assets/noise_bg.npy");

    int  mask_size_ratio = 2; // 8, 4, 2, 1
    int  resize_amount   = 1;
    int  upscale_amount  = 1;
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
    e23_global::dist = model.m_dist;
    double lr;
#if defined(OPT_ADAM)
    if (model.m_dist->get_name() == "categorical" ||
        model.m_dist->get_name() == "binary") {
        std::cout << "INFO: [e23] Using Adam optimizer with Categorical or Binary distribution.\n";
        lr = 0.5f;
    }
    else {
        std::cout << "INFO: [e23] Using Adam optimizer with Normal distribution.\n";
        lr = 0.1f;
    }
    torch::optim::Adam opt_m (model.parameters(), torch::optim::AdamOptions(lr));
#else
    lr = 10.0f;
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
        300.0f, /* Exposure Time */
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
        model_Height * 2 * upscale_amount,
        model_Width  * 2 * upscale_amount,

        /* Optimizer */
        &opt,

        /* Processing function */
        process_function
    );
    scheduler.EnableSampleImageCapture();
    scheduler.SetRewardDevice(DEVICE);
    scheduler.EnableLabelQueueing();
    scheduler.EnableBlendMode();    /* For use with DLP/PLM system, disable if only PLM */
    scheduler.EnableFullScreenSubTextures();
    if (model.m_dist->get_name() == "categorical") {
        std::cout << "INFO: [e23] Using Categorical distribution for the model.\n";
        scheduler.EnableCategoricalMode(); // For Categorical distribution
    } else if (model.m_dist->get_name() == "normal") {
        std::cout << "INFO: [e23] Using Normal distribution for the model.\n";
    } else if (model.m_dist->get_name() == "bernoulli" || model.m_dist->get_name() == "binary") {
        std::cout << "INFO: [e23] Using Binary distribution for the model.\n";
        scheduler.EnableBinaryMode(); // For Binary distribution
    }
    else {
        std::cout << "INFO: [e23] Using unknown distribution (" << model.m_dist->get_name() << ") for the model.\n";
    }
    opt.epsilon = 0.05;

    // Parameters
    int64_t n_training_samples = 1000;
    int64_t n_batch_size       = 20;
    int64_t n_samples          = 10;    // Note actual number of samples is n_samples * 20
    //int64_t n_samples = 5; // 5 samples per image, total 100 frames per image
    bool    antithetic         = false;
    int64_t data_padding_amount = 5;
    float sub_shader_threshold = 0.8;

    scheduler.SetSubShaderThreshold(sub_shader_threshold);
    e23_global::antithetic = antithetic;
    /*
    if (antithetic) {
        scheduler.EnableAntiTheticSampling();
    }
    */

    auto batches = Get_Data(n_training_samples, n_batch_size, s2_DataTypes::TRAIN, data_padding_amount);
    scheduler.SetBatchSize(n_batch_size);
    
    int64_t n_validation_samples  = 1000;
    int64_t n_validation_batch_size = 1000;
    auto val_batches = Get_Data(n_validation_samples, n_validation_batch_size, s2_DataTypes::VALID, data_padding_amount);
    //auto val_batches = batches;

    int64_t step=0;
    int64_t batch_sel=0;

    double  mean_reward = 0.0f;
    double  mean_val_reward = 0.0f;
    double  val_reward = 0.0f;

    std::string cp_dir = "./2025_10_20_002_s16-1";
    int64_t cp_interval = batches.size(); // in number batches
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
        ofs << "Resize ratio: " << resize_amount << '\n';
        ofs << "Data Padding Amount: " << data_padding_amount << '\n';
        ofs << "Sub-shader threshold: " << sub_shader_threshold << '\n';
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
        ofs << (scheduler.IsBlendModeEnabled() ? "Blend mode enabled" : "Blend mode disabled") << '\n';
        ofs << "Type of distribution used for model: " << model.m_dist->get_name() << '\n';
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

        throw std::runtime_error("Checkpoint loading not supported in this.");

        // Set model
        /*
        model.set_params(
            cp.phase,
            cp.kappa
        );
        */

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

    double training_correct_f = 0.0f;
    double training_count_f   = 0.0f;

    double validation_correct_f = 0.0f;
    double validation_count_f   = 0.0f;

    int64_t start_time = Utils::GetCurrentTime_s ();
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

            action = Utils::UpscaleTensor(action, resize_amount);

            scheduler.SetTextureFromTensorTiled(action);
            //action = torch::fft::fftshift(action, {-2, -1});

            if (i == 1) {
                training_accuracy = 1000.0f * e23_global::correct.load(std::memory_order_acquire) / e23_global::total.load(std::memory_order_acquire);
                training_correct_f += static_cast<double>(e23_global::correct.load(std::memory_order_acquire));
                training_count_f   += static_cast<double>(e23_global::total.load(std::memory_order_acquire));
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
        //auto reward = scheduler.UpdatePPO();
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

            std::cout << "INFO: [e23] Running Validation...\n";
            torch::Tensor action = model.m_dist->base(scheduler.maximum_number_of_frames_in_image);
            std::cout << "INFO: [e23] Validation action size: " << action.sizes() << '\n';

            action = Utils::UpscaleTensor(action, resize_amount);

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
            val_reward = scheduler.Loss();
            validation_accuracy = 1000 * e23_global::correct.load(std::memory_order_acquire) / e23_global::total.load(std::memory_order_acquire);

            validation_correct_f = static_cast<double>(e23_global::correct.load(std::memory_order_acquire));
            validation_count_f   = static_cast<double>(e23_global::total.load(std::memory_order_acquire));

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

        /////////////////////////////////////////////////////
        // Saving checkpoints                              //
        /////////////////////////////////////////////////////
        if (save_checkpoints && (iter % cp_interval == (cp_interval - 1))) {
            Scheduler2_CheckPoint cp;
            cp.batch_id = batch_sel;
            cp.training_accuracy = training_accuracy;
            cp.validation_accuracy = validation_accuracy;
            cp.val_reward = mean_val_reward;
            cp.phase = model.get_parameters();
            cp.kappa = 1/model.std;
            cp.step = step;
            cp.dataset_path = "./Datasets";
            cp.checkpoint_dir = cp_dir;
            cp.checkpoint_name = "";
            cp.reward = reward;
            scheduler.SaveCheckpoint(cp);
        }
        ++iter;

        batch_sel = (batch_sel + 1) % batches.size();

        mean_reward += reward;
        mean_val_reward = val_reward;

        if (batch_sel == 0) {
            mean_reward /= static_cast<double>(batches.size());
            //mean_val_reward /= static_cast<double>(val_batches.size());
            // Save mean reward to file (append)
            std::ofstream ofs(cp_dir + "/a/mean_reward.txt", std::ios::app);
            ofs << "Epoch: " << iter / batches.size() << '\n';
            ofs << "Training Loss: "  << mean_reward << ", Validation Loss: " << mean_val_reward;
            ofs << ", Training Acc: " << 100.0f * (training_correct_f / training_count_f) << "%, Validation Acc: " << 100.0f * (validation_correct_f / validation_count_f) << "%\n";

            // Save entropy of distribution
            ofs << "Entropy: " << e23_global::dist->entropy_no_grad().mean().item<double>() << '\n';

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

            int64_t current_time = Utils::GetCurrentTime_s();
            ofs << "\nElapsed Time (s): " << (current_time - start_time) << '\n';
            ofs << "\n---\n";

            mean_reward = 0.0f;
            mean_val_reward = 0.0f;
            training_correct_f = 0.0f;
            training_count_f = 0.0f;
            validation_correct_f = 0.0f;
            validation_count_f = 0.0f;

            // Clear both
            for (int i = 0; i < 10; ++i) {
                training_label_counts[i] = 0;
                training_label_freq[i] = 0;
                validation_label_counts[i] = 0;
                validation_label_freq[i] = 0;
            }
        }

        if (model.get_parameters().isnan().any().item<bool>() ||
            model.get_parameters().isinf().any().item<bool>()) {
            std::cout << "WARNING: [e23] Model parameters contain NaN or Inf. Exiting...\n";
            throw std::runtime_error("Model parameters contain NaN or Inf.");
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