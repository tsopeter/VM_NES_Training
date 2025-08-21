#include "e23.hpp"

#include <torch/torch.h>

#include "../s4/model.hpp"
#include "../s4/optimizer.hpp"
#include "../s5/scheduler2.hpp"
#include "../s5/hcomms.hpp"
#include "../utils/utils.hpp"
#include "../s4/utils.hpp"
#include "../s2/dataloader.hpp"

namespace e23_global {
    static std::atomic<double> correct = 0;
    static std::atomic<double> total   = 0;
}

struct e23_Batch {
    std::vector<Texture> textures;
    std::vector<int>     labels;
};


struct e23_Validation_Batch {
    Texture texture;
    std::vector<int> labels;
};


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

            auto li = l.argmax().item<int64_t>();

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

    void init (int64_t Height, int64_t Width, int64_t n) {
        m_Height = Height;
        m_Width  = Width;
        m_n      = n;

        std::cout<<"INFO: [e23_Model] Staging model...\n";
        std::cout<<"INFO: [e23_Model] Height: " << Height << ", Width: " << Width << ", Number of Perturbations (samples): " << n << '\n';

        m_parameter = torch::rand({Height, Width}).to(DEVICE) * 2 * M_PI - M_PI;  /* Why does placing m_parameter on CUDA cause segmentation fault */

        /*        
        torch::Tensor mask = torch::ones({m_Height, m_Width});

        // Create a pattern in the mask, where alternating rows of size 10 are 1 and 0
        for (int i = 0; i < m_Height; i += 10) {
            if (i % 20 == 0) {
                mask.index_put_({torch::indexing::Slice(i, i + 10), torch::indexing::Slice()}, 0.0f);
            }
        }
        m_parameter = s4_Utils::GSAlgorithm(mask, 50).to(torch::kFloat32).to(DEVICE);
        */

        // save m_parameter to disk
        //std::cout<<"INFO: [e23_Model] Created parameter tensor of shape: " << m_parameter.sizes() << '\n';
        //torch::save({m_parameter.cpu().detach()}, "e23_parameter.pt");

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

    const double std = 0.02;
    const double kappa = 1.0f/std;

    std::vector<torch::Tensor> m_action_s;  /* Used for sequential creation. */
};


std::pair<torch::Tensor, bool> e23_ProcessFunction (CaptureData &ts) {
    static double norm_factors[] = {1, 1, 1, 1, 1, 
                                    1, 1, 1, 1, 1};
    static double gain = 2;
    static double gain_factor = 0.9995;
    static int64_t process_count = 0;

    std::cout << "INFO: [e23_ProcessFunction]: Processed: " << ++process_count << " so far.\n";

    // Sum to [16]
    auto t = ts.image;
    auto k = ts.full; // full image
    auto l = torch::tensor({ts.label}); // label

    auto sums = t.sum({1,2});
    auto ksum = k.sum() - sums.sum();

    std::vector<torch::Tensor> t_vec;

    for (int i = 0; i < 10; ++i) {
        t_vec.push_back(gain * norm_factors[i] * sums[i].unsqueeze(0).to(torch::kFloat64));
    }

    // The first instance is used as a normalization factor
    static bool first_instance = true;
    if (first_instance) {
        // compute normalization factors
        for (int i = 0; i < 10; ++i) {
            norm_factors[i] = 1.0 / (sums[i].item<double>() + 1e-8);
        }

        first_instance = false;
    }

    // Concatenate along the first dimension
    auto predictions = torch::stack(t_vec, 1);  // [1, 10]
    // take the argmax
    auto preds = predictions.argmax(1); // [1]

    if (preds.item<int64_t>() == l.item<int64_t>()) {
        e23_global::correct.fetch_add(1, std::memory_order_release);
    }
    e23_global::total.fetch_add(1, std::memory_order_release);

    auto targets = l.to(torch::kLong).to(t.device()); // [1]

    // score is the cross entropy loss
    auto loss = torch::nn::functional::cross_entropy(
        predictions,
        targets,
        torch::nn::functional::CrossEntropyFuncOptions().reduction(torch::kNone)
    );  // [1]

    return {-loss, true};
}

int e23 () {
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
    model.init(200/2, 320/2, scheduler.maximum_number_of_frames_in_image);
    torch::optim::Adam adam (model.parameters(), torch::optim::AdamOptions(0.01));
    s4_Optimizer opt (adam, model);

    HComms comms {"192.168.193.20", 9001};

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
        59.0f,
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
        1600/8,
        2560/8,

        /* Optimizer */
        &opt,

        /* Processing function */
        process_function
    );
    scheduler.EnableSampleImageCapture();
    scheduler.SetRewardDevice(DEVICE);

    // Get image data
    int64_t n_training_samples = 100;
    int64_t n_batch_size       = 10;
    int64_t n_samples          = 16;    // Note actual number of samples is n_samples * 20

    auto batches = Get_Data(n_training_samples, n_batch_size, s2_DataTypes::TRAIN);
    scheduler.SetBatchSize(n_batch_size);

    
    int64_t n_validation_samples = 20;
    std::cout << "INFO: [e23] Loading Validation Set\n";
    std::vector<e23_Validation_Batch> val_batches = e23_Pack20(n_validation_samples/20); // note that we obtain in batches of 20
    std::cout << "INFO: [e23] Loaded " << val_batches.size() << " validation batches.\n";
    

    int64_t step=0;
    int64_t batch_sel=0;
    while (!WindowShouldClose()) {
        int64_t time_0 = Utils::GetCurrentTime_us();

        for (int i =0; i < n_samples; ++i) {
            torch::Tensor action = model.sample(scheduler.maximum_number_of_frames_in_image);
            scheduler.SetTextureFromTensorTiled(action);

            for (int j = 0; j < n_batch_size; ++j) {
                scheduler.SetSubTextures(batches[batch_sel].textures[j], 0);
                scheduler.SetLabel(batches[batch_sel].labels[j]);

                scheduler.DrawTextureToScreenTiled();
                scheduler.DrawTextureToScreenTiled();
                scheduler.ReadFromCamera();
                ++step;
            }
        }

        TakeScreenshot("train_screen.png");

        batch_sel = (batch_sel + 1) % batches.size();
        std::cout << "INFO: [e23] Training step " << step << " completed...\n";

        model.squash();
        auto reward = scheduler.Update();
        int64_t time_1 = Utils::GetCurrentTime_us();

        int64_t delta = time_1 - time_0;

        e23_global::correct.store(0, std::memory_order_release);
        e23_global::total.store(0, std::memory_order_release);
        auto mask = model.get_parameters(); // [H, W]
        scheduler.Validation_SetMask(mask);
        scheduler.Validation_SetTileParams(8);
        // Draw the validation set

        std::cout << "INFO: [e23] Running Validation...\n";
        for (int i = 0; i < val_batches.size(); ++i) {
            scheduler.SetLabel(val_batches[i].labels);
            scheduler.Validation_SetDatasetTexture(val_batches[i].texture);
            scheduler.Validation_DrawToScreen();
            scheduler.Validation_DrawToScreen();
            scheduler.ReadFromCamera();
        }
        // Export the screen shot for analysis
        scheduler.Validation_SaveMaskToDrive("val_mask.png");
        scheduler.Training_SaveMaskToDrive("train_mask.png");
        TakeScreenshot("val_screen.png");

        scheduler.Dump(20);

        // Transmit the data to remote server
        HCommsDataPacket_Outbound packet;
        packet.reward = 100 * e23_global::correct.load(std::memory_order_acquire) / e23_global::total.load(std::memory_order_acquire);
        packet.step   = step;
        packet.image  = scheduler.GetSampleImage().contiguous().to(torch::kUInt8);

        // Send the packet
        comms.Transmit(packet);
        scheduler.DisposeSampleImages();

        e23_global::correct.store(0, std::memory_order_release);
        e23_global::total.store(0, std::memory_order_release);
    }

    scheduler.StopThreads();
    scheduler.StopCamera();
    scheduler.StopWindow();

    for (auto &b : batches) {
        for (auto &t : b.textures) {
            UnloadTexture(t);
        }
    }

    
    for (auto &t : val_batches) {
        UnloadTexture(t.texture);
    }
    
    return 0;
}