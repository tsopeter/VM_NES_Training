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
    static double norms[10] = {0};
    static double gains     = 1.0;
    static torch::Tensor noise_bg = torch::tensor(
        std::vector<double>{837, 765, 714, 788, 353, 342, 314, 371, 365, 354,
        0, 0, 0, 0, 0, 0},
        torch::dtype(torch::kFloat64)
    ).reshape({16});
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

std::vector<e23_Validation_Batch> e23_Pack20 (int n) {
    s2_Dataloader dataloader {"./Datasets"};
    std::vector<e23_Validation_Batch> batches;

    auto dl = dataloader.load(s2_DataTypes::VALID, n*20);

    for (int i = 0; i < n * 20; i += 20) {
        uint8_t data[28 * 28 * 20]; // 1 channel, R32B32G32A32, 20 images
        std::vector<int> labels;

        // Each image is 28x28 of uint8_t
        for (int j = 0; j < 20; ++j) {
            auto [image, l] = dl[i + j];
            labels.push_back(l.argmax().item<int>());
            image = image.to(torch::kUInt8).contiguous(); // [28 x 28]

            // copy sequentially
            std::memcpy(data + j * 28 * 28, image.data_ptr<uint8_t>(), 28 * 28);

        }
        Image image;
        image.data = data;
        image.width = 28;
        image.height = 28 * 20;
        image.mipmaps = 1;
        image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;

        Texture texture = LoadTextureFromImage(image);
        SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR); 
        batches.push_back(
            e23_Validation_Batch{.texture = texture, .labels = labels}
        );
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

        m_parameter = torch::rand({Height, Width}).to(DEVICE) * 2 * M_PI - M_PI;
        //m_parameter = torch::zeros({Height, Width}).to(DEVICE);
        m_parameter.set_requires_grad(true);
        std::cout<<"INFO: [e23_Model] Set parameters...\n";

        m_dist.set_mu(m_parameter, kappa);
        std::cout<<"INFO: [e23_Model] Created distribution...\n";
    }

    ~e23_Model () override {}

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
    e23_Normal m_dist {};

    const double std = 0.1 * (2 * M_PI); // 10% of the range
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
    // so we can divide each zone into 4 areas of 30x30
    auto lb_0 = t[5].slice(0, 0, 30).slice(1, 0, 30);   // get label 0
    auto lb_1 = t[5].slice(0, 0, 30).slice(1, 30, 60);  // get label 1
    auto lb_2 = t[5].slice(0, 30, 60).slice(1, 0, 30);  // get label 2
    auto lb_3 = t[5].slice(0, 30, 60).slice(1, 30, 60); // get label 3

    auto lb_4 = t[6].slice(0, 0, 30).slice(1, 0, 30);   // get label 4
    auto lb_5 = t[6].slice(0, 0, 30).slice(1, 30, 60);  // get label 5
    auto lb_6 = t[6].slice(0, 30, 60).slice(1, 0, 30);  // get label 6
    auto lb_7 = t[6].slice(0, 30, 60).slice(1, 30, 60); // get label 7

    auto lb_8 = t[10].slice(0, 0, 30).slice(1, 0, 30);   // get label 8
    auto lb_9 = t[10].slice(0, 0, 30).slice(1, 30, 60); // get label 9


    // get label 0 and 1

    // Cat together
    t = torch::stack({lb_0, lb_1, lb_2, lb_3, lb_4, lb_5, lb_6, lb_7, lb_8, lb_9}, 0); // [10, 30, 60]

    auto sums = t.sum({1,2}).to(torch::kFloat64);   // [16]

    //sums = sums - e23_global::noise_bg;

    // Each area is 60x60 = 3600 pixels
    sums /= 255.0;

    auto ksum = k.sum() - sums.sum();

    std::vector<torch::Tensor> t_vec;

    if (e23_global::enable_norm)
        for (int i = 0; i < 10; ++i) {
            t_vec.push_back(e23_global::gains * e23_global::norms[i] * sums[i].unsqueeze(0).to(torch::kFloat64));
        }
    else
        for (int i = 0; i < 10; ++i) {
            t_vec.push_back(e23_global::gains * sums[i].unsqueeze(0).to(torch::kFloat64));
        }

    if (e23_global::acc_norm) {
        // Sum up the values
        for (int i = 0; i < 10; ++i) {
            e23_global::norms[i] += t_vec[i].squeeze().item<double>();
        }
    }

    // Concatenate along the first dimension
    auto predictions = torch::stack(t_vec, 1);  // [1, 10]



    // take the argmax
    auto preds = predictions.argmax(1); // [1]

    if (process_count % 20 == 0) {
        if (preds.item<int>() == ts.label) {
            e23_global::correct.fetch_add(1, std::memory_order_release);
        }
        e23_global::total.fetch_add(1, std::memory_order_release);

        if (start_saving_data_points.load(std::memory_order_acquire)) {
            e23_DataPoints dp;
            dp.label = ts.label;
            for (int i = 0; i < 10; ++i) {
                dp.results[i] = predictions[0][i].item<double>();
            }
            data_points.push_back(dp);
        }
    }

    auto targets = l.to(torch::kLong).to(t.device()); // [1]

    // score is the cross entropy loss
    auto loss = torch::nn::functional::cross_entropy(
        predictions,
        targets,
        torch::nn::functional::CrossEntropyFuncOptions().reduction(torch::kNone)
    );  // [1]

    ++process_count;
    return {-loss, true};
}

int e23 () {
    int  mask_size_ratio = 4;
    bool load_from_checkpoint = false;
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

    model.init(model_Height, model_Width, scheduler.maximum_number_of_frames_in_image);
    torch::optim::Adam opt_m (model.parameters(), torch::optim::AdamOptions(0.05));
    //torch::optim::SGD opt_m (model.parameters(), torch::optim::SGDOptions(100.0f));
    s4_Optimizer opt (opt_m, model);

    HComms comms {"192.168.193.136", 9001};

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
        150.0f, /* Exposure Time */
        1,  /* Binning Horizontal */
        1,  /* Binning Vertical */
        3,  /* Line Trigger */
        use_partitioning,  /* Use Zones */
        4,  /* Number of Zones */
        60, /* Zone Size */
        true, /* Use Centering */
        0, /* Offset X */
        0, /* Offset Y */

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
    int64_t n_training_samples = 640;
    int64_t n_batch_size       = 32;
    int64_t n_samples          = 16;    // Note actual number of samples is n_samples * 20

    auto batches = Get_Data(n_training_samples, n_batch_size, s2_DataTypes::TRAIN);
    scheduler.SetBatchSize(n_batch_size);

    
    int64_t n_validation_samples  = 32;
    int64_t n_validation_batch_size = 32;
    auto val_batches = Get_Data(n_validation_samples, n_validation_batch_size, s2_DataTypes::TRAIN);
    //auto val_batches = batches;

    int64_t step=0;
    int64_t batch_sel=0;

    double  mean_reward = 0.0f;

    auto Iterate = [&scheduler]->void {
        for (int i = 0; i < 2; ++i) {
            scheduler.DrawTextureToScreenTiled();
            //scheduler.DrawTextureToScreenCentered();

            scheduler.SetVSYNC_Marker();
            scheduler.WaitVSYNC_Diff(1);
        }
        scheduler.ReadFromCamera();
    };


    /////////////////////////////////////////////////////
    // Obtaining Normalization Factor                  //
    /////////////////////////////////////////////////////
    
    /*
    e23_global::acc_norm.store(true, std::memory_order_release);
    e23_global::enable_norm.store(false, std::memory_order_release);
    torch::Tensor action = model.sample(scheduler.maximum_number_of_frames_in_image);
    scheduler.SetTextureFromTensorTiled(action);
    for (int i = 0; i < n_validation_batch_size; ++i) {
        scheduler.SetSubTextures(val_batches[0].textures[i], 0);
        Iterate();
    }
    model.squash();
    scheduler.Dump();

    // calculate norm
    for (int i = 0; i < 10; ++i) {
        e23_global::norms[i] = static_cast<double>(n_validation_batch_size) / e23_global::norms[i];
    }

    e23_global::acc_norm.store(false, std::memory_order_release);
    e23_global::enable_norm.store(true, std::memory_order_release);
    */

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
        batch_sel = cp.batch_id;

        // set the step
        step = cp.step;
    }


    int64_t iter = 0;
    const int64_t val_interval = 1;
    int validation_accuracy = 0;
    int training_accuracy = 0;

    while (!WindowShouldClose()) {

        /////////////////////////////////////////////////////
        // Training                                        //
        /////////////////////////////////////////////////////

        int64_t time_0 = Utils::GetCurrentTime_us();

        for (int i =0; i < n_samples; ++i) {
            torch::Tensor action = model.sample(scheduler.maximum_number_of_frames_in_image);
            scheduler.SetTextureFromTensorTiled(action);

            for (int j = 0; j < n_batch_size; ++j) {
                scheduler.SetSubTextures(batches[batch_sel].textures[j], 0);
                scheduler.SetLabel(batches[batch_sel].labels[j]);
                Iterate();
                ++step;
            }
        }

        //TakeScreenshot("train_screen.png");
        std::cout << "INFO: [e23] Training step " << step << " completed...\n";

        model.squash();
        auto reward = scheduler.Update();
        //scheduler.Dump();
        int64_t time_1 = Utils::GetCurrentTime_us();

        int64_t delta = time_1 - time_0;
        training_accuracy = 1000 * e23_global::correct.load(std::memory_order_acquire) / e23_global::total.load(std::memory_order_acquire);

        /////////////////////////////////////////////////////
        // Validation                                      //
        /////////////////////////////////////////////////////
        
        e23_global::correct.store(0, std::memory_order_release);
        e23_global::total.store(0, std::memory_order_release);

        if (iter % val_interval == 0) {
            // Get std from model
            double std = model.m_dist.get_std();
            model.m_dist.set_std(0.0f);

            std::cout << "INFO: [e23] Running Validation...\n";
            torch::Tensor action = model.sample(scheduler.maximum_number_of_frames_in_image);
            scheduler.SetTextureFromTensorTiled(action);

            if (iter == 0) {
                start_saving_data_points.store(true, std::memory_order_release);
            }


            for (int i = 0; i < val_batches.size(); ++i) {
                for (int j = 0; j < n_validation_batch_size; ++j) {
                    scheduler.SetSubTextures(val_batches[i].textures[j], 0);
                    scheduler.SetLabel(val_batches[i].labels[j]);
                    Iterate();
                }
            }
            // set the std back
            model.squash();
            model.m_dist.set_std(std);
            scheduler.Dump();
            validation_accuracy = 1000 * e23_global::correct.load(std::memory_order_acquire) / e23_global::total.load(std::memory_order_acquire);


            start_saving_data_points.store(false, std::memory_order_release);
            // save
            if (iter == 0) {
                e23_SaveDataPoints();
            }
        }
        
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
        HCommsDataPacket_Outbound packet;
        packet.reward = combined_accuracy; // = reward;
        packet.step   = step;
        packet.image  = scheduler.GetSampleImage().contiguous().to(torch::kUInt8);

        // Send the packet
        comms.Transmit(packet);
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
            cp.phase = model.get_parameters();
            cp.kappa = 1/model.m_dist.get_std();
            cp.step = step;
            cp.dataset_path = "./Datasets";
            cp.checkpoint_dir = "./2025_09_09_002";
            cp.checkpoint_name = "";
            cp.reward = reward;
            scheduler.SaveCheckpoint(cp);
        }

        batch_sel = (batch_sel + 1) % batches.size();

        mean_reward += reward;

        if (batch_sel == 0) {
            mean_reward /= static_cast<double>(batches.size());
            // Save mean reward to file (append)
            std::ofstream ofs("./2025_09_09_002/mean_reward.txt", std::ios::app);
            ofs << mean_reward << std::endl;
            mean_reward = 0.0f;
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
    return 0;
}