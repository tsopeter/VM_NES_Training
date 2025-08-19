#include "e23.hpp"

#include <torch/torch.h>

#include "../s4/model.hpp"
#include "../s4/optimizer.hpp"
#include "../s5/scheduler2.hpp"
#include "../s5/hcomms.hpp"
#include "../utils/utils.hpp"
#include "../s4/utils.hpp"
#include "../s2/dataloader.hpp"


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

    const double std = 0.05;
    const double kappa = 1.0f/std;

    std::vector<torch::Tensor> m_action_s;  /* Used for sequential creation. */
};


std::pair<torch::Tensor, bool> e23_ProcessFunction (CaptureData &ts) {
    // Sum to [16]
    auto t = ts.image;
    auto k = ts.full; // full image
    auto l = torch::tensor({ts.label}); // label

    auto sums = t.sum({1,2});
    auto ksum = k.sum() - sums.sum();

    // just one
    auto t0 = sums[0].unsqueeze(0).to(torch::kFloat64);
    auto t1 = sums[1].unsqueeze(0).to(torch::kFloat64);

    // rest (for noise suppression)
    auto t2 = torch::sum(sums.index({torch::indexing::Slice(2,16)}), 0).unsqueeze(0).to(torch::kFloat64);
    t2 = t2 + ksum;

    // Concatenate along the first dimension
    auto predictions = torch::stack({t0, t1, t2}, 1);  // [1, 3]

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
    model.init(200, 320, scheduler.maximum_number_of_frames_in_image);
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
        1600/4,
        2560/4,

        /* Optimizer */
        &opt,

        /* Processing function */
        process_function
    );
    scheduler.EnableSampleImageCapture();
    scheduler.SetRewardDevice(DEVICE);

    // Get image data
    s2_Dataloader data_loader {"./Datasets/"};
    auto data = data_loader.load(s2_DataTypes::TRAIN, 10);
    std::cout << "INFO: [e23] Loaded data with " << data.len() << " samples.\n";

    auto [d0, l0] = data[0];
    auto [d1, l1] = data[1];

    d0 = 255.0 - d0;
    d1 = 255.0 - d1;
    Image i0 = s4_Utils::TensorToImage(d0);
    Image i1 = s4_Utils::TensorToImage(d1);

    // Save image
    //ExportImage(i0, "circle.bmp");

    // Set each image to use bilinear
    Texture t0 = LoadTextureFromImage(i0);
    Texture t1 = LoadTextureFromImage(i1);
    SetTextureFilter(t0, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(t1, TEXTURE_FILTER_BILINEAR);

    scheduler.SetSubTextures(t0, 0);
    scheduler.EnableSubTexture(0);
    scheduler.SetSubTextures(t1, 1);
    scheduler.DisableSubTexture(1);

    auto draw_to_screen_with_i = [&](int index, torch::Tensor &action) {
        for (int i = 0; i < 10; ++i)
            scheduler.DisableSubTexture(i);
        scheduler.EnableSubTexture(index);
        scheduler.SetLabel(index);

        if (use_tiles) {
            scheduler.SetTextureFromTensorTiled(action);
            scheduler.DrawTextureToScreenTiled();
            scheduler.DrawTextureToScreenTiled();
        } else {
            scheduler.SetTextureFromTensor(action);
            scheduler.DrawTextureToScreen();
            scheduler.DrawTextureToScreen();
        }
    };

    int64_t step=0;
    while (!WindowShouldClose()) {
        int64_t time_0 = Utils::GetCurrentTime_us();


        for (int i =0; i < 16; ++i) {
            torch::Tensor action = model.sample(scheduler.maximum_number_of_frames_in_image);
            draw_to_screen_with_i(0, action);
            scheduler.ReadFromCamera();

            draw_to_screen_with_i(1, action);
            scheduler.ReadFromCamera();
            ++step;
        }



        model.squash();
        auto reward = scheduler.Update();
        int64_t time_1 = Utils::GetCurrentTime_us();

        int64_t delta = time_1 - time_0;

        // Transmit the data to remote server
        
        HCommsDataPacket_Outbound packet;
        packet.reward = reward;
        packet.step   = step;
        packet.image  = scheduler.GetSampleImage().contiguous().to(torch::kUInt8);

        // Send the packet
        comms.Transmit(packet);
        scheduler.DisposeSampleImages();
    }

    scheduler.StopThreads();
    scheduler.StopCamera();
    scheduler.StopWindow();

    UnloadTexture(t0);
    UnloadTexture(t1);
    UnloadImage(i0);
    UnloadImage(i1);

    return 0;
}