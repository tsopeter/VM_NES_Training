#include "e23.hpp"

#include <torch/torch.h>

#include "../s4/model.hpp"
#include "../s4/optimizer.hpp"
#include "../s5/scheduler2.hpp"
#include "../s5/hcomms.hpp"
#include "../utils/utils.hpp"
#include "../s4/utils.hpp"


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

        //m_parameter = torch::rand({Height, Width}).to(DEVICE) * 2 * M_PI - M_PI;  /* Why does placing m_parameter on CUDA cause segmentation fault */

        torch::Tensor mask = torch::ones({m_Height, m_Width});

        // Create a pattern in the mask, where alternating rows of size 10 are 1 and 0
        for (int i = 0; i < m_Height; i += 10) {
            if (i % 20 == 0) {
                mask.index_put_({torch::indexing::Slice(i, i + 10), torch::indexing::Slice()}, 0.0f);
            }
        }

        m_parameter = s4_Utils::GSAlgorithm(mask, 50).to(torch::kFloat32).to(DEVICE);

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

    const double std = 0.5;//1e-0;
    const double kappa = 1.0f/std;

    std::vector<torch::Tensor> m_action_s;  /* Used for sequential creation. */
};


torch::Tensor e23_ProcessFunction (torch::Tensor &t) {
    if (t.dim() == 3) {
        // Use

        auto zone_0    = t.index({torch::indexing::Slice(4, 7), torch::indexing::Slice(), torch::indexing::Slice()});

        // Get Channels 0,4,8,12
        // These are the target channels
        auto zone_0_4_8_12 = t.index({torch::indexing::Slice(0, 16, 4), torch::indexing::Slice(), torch::indexing::Slice()});

        // Get channels 1,2,3,5,6,7,9,10,11,13,14,15
        // These are the non-target channels
        torch::Tensor zone_1_2_3_5_6_7_9_10_11_13_14_15;
        zone_1_2_3_5_6_7_9_10_11_13_14_15 = torch::cat({
            t.index({torch::indexing::Slice(1, 16, 4), torch::indexing::Slice(), torch::indexing::Slice()}),
            t.index({torch::indexing::Slice(2, 16, 4), torch::indexing::Slice(), torch::indexing::Slice()}),
            t.index({torch::indexing::Slice(3, 16, 4), torch::indexing::Slice(), torch::indexing::Slice()})
        }, 0);


        auto t0 = zone_0_4_8_12.sum().unsqueeze(0).to(torch::kFloat64); // [1]
        auto t1 = zone_1_2_3_5_6_7_9_10_11_13_14_15.sum().unsqueeze(0).to(torch::kFloat64); // [1]

        // Concatenate along the first dimension
        auto predictions = torch::stack({t0, t1}, 1);  // [1, 2]

        // the first class is always the target
        auto targets = torch::zeros({1}, torch::kLong).to(t.device());  // [1]

        // score is the cross entropy loss
        auto loss = torch::nn::functional::cross_entropy(
            predictions,
            targets,
            torch::nn::functional::CrossEntropyFuncOptions().reduction(torch::kNone)
        );  // [1]

        return -loss;
    }
    else {
        int64_t H = t.size(0);
        int64_t W = t.size(1);
        // Use more primative processing
        torch::Tensor t0 = torch::zeros({H, W}, torch::kFloat64);
        t0.index_put_({torch::indexing::Slice(), torch::indexing::Slice(W/2-8, W/2+8)}, 1.0f);

        torch::Tensor t1 = 1.0 - t0; // Invert the tensor
        
        // Stack the tensors
        auto mask = torch::stack({t0, t1}, 0).unsqueeze(0);
        t = t.unsqueeze(0);  // [1, H, W]

        auto scores = (t * mask).sum({2, 3}); // [1, 2]

        torch::Tensor targets = torch::zeros({1}, torch::kLong).to(t.device());  // [1]

        // apply cross entropy loss
        auto loss = torch::nn::functional::cross_entropy(
            scores,
            targets,
            torch::nn::functional::CrossEntropyFuncOptions().reduction(torch::kNone)
        );  // [1]

        return -loss;
    }
}

int e23 () {
    /* Camera Parameters */
    int Height = 480, Width = 640;
    bool use_partitioning = false;

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
    model.init(200*2, 320*2, scheduler.maximum_number_of_frames_in_image);
    torch::optim::Adam adam (model.parameters(), torch::optim::AdamOptions(0.1));
    s4_Optimizer opt (adam, model);

    HComms comms {"192.168.193.20", 9001};

    auto process_function = [](torch::Tensor t)->torch::Tensor {
        return e23_ProcessFunction(t);
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
        false, /* Use Centering */
        0, /* Offset X */
        0, /* Offset Y */

        /* Optimizer */
        &opt,

        /* Processing function */
        process_function
    );
    scheduler.EnableSampleImageCapture();
    scheduler.SetRewardDevice(DEVICE);

    
    int64_t step=0;
    while (!WindowShouldClose()) {
        int64_t time_0 = Utils::GetCurrentTime_us();
        for (int i =0; i < 10; ++i) {
            torch::Tensor action = model.sample(scheduler.maximum_number_of_frames_in_image);
            scheduler.SetTextureFromTensor(action);
            scheduler.DrawTextureToScreen();
            scheduler.DrawTextureToScreen();
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

    return 0;
}