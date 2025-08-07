#include "e23.hpp"

#include <torch/torch.h>

#include "../s4/model.hpp"
#include "../s4/optimizer.hpp"
#include "../s5/scheduler2.hpp"
#include "../s5/hcomms.hpp"


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
        return m_n;
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



int e23 () {
    Pylon::PylonAutoInitTerm init {};
    Scheduler2 scheduler {};

    e23_Model model {};
    model.init(200*2, 320*2, scheduler.maximum_number_of_frames_in_image);
    torch::optim::Adam adam (model.parameters(), torch::optim::AdamOptions(0.1));
    s4_Optimizer opt (adam, model);

    HComms comms {"192.168.193.20", 9001};

    // Set the target
    torch::Tensor target_0 = torch::zeros({240, 320}, torch::kFloat32);
    target_0.index_put_(
        {torch::indexing::Slice(),
            torch::indexing::Slice(scheduler.camera.Width/2 - 8, scheduler.camera.Width/2 + 8)},
        1.0f
    );

    auto target_1 = 1.0 - target_0;
    auto regions  = torch::stack({target_0, target_1});

    auto process_function = [regions](torch::Tensor t) {
        // t would have shape [N, H, W]

        if (t.dim() == 2) {
            t = t.unsqueeze(0);  // convert to [1, H, W] 
        }

        auto t_expanded = t.unsqueeze(1); // [B, 1, H, W]
        auto m_regions_expanded = regions.unsqueeze(0).to(t.device()); // [1, N, H, W]
        auto scores = (t_expanded * m_regions_expanded).sum({2, 3}); // [B, N]

        return scores;
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
        240,
        320,
        59.0f,
        1,  /* Binning Horizontal */
        1,  /* Binning Vertical */

        /* Optimizer */
        &opt,

        /* Processing function */
        process_function
    );

    
    int64_t step=0;
    while (!WindowShouldClose()) {
        for (int i =0; i < 5; ++i) {
            torch::Tensor action = model.sample(scheduler.maximum_number_of_frames_in_image);
            scheduler.SetTextureFromTensor(action);
            scheduler.DrawTextureToScreen();
            scheduler.DrawTextureToScreen();
            scheduler.ReadFromCamera();
            ++step;
        }
        model.squash();
        //auto reward = scheduler.Update();

        HCommsDataPacket_Outbound packet;
        packet.step = step;
        //packet.reward = reward;
        packet.reward = 0.0;
        packet.image = scheduler.GetSampleImage().to(torch::kUInt8);
        comms.Transmit(packet);

        
        // Save a sample image
        //scheduler.SaveSampleImage("sample_image.png");
        //break;
        
        scheduler.DisposeSampleImages();
    }

    scheduler.StopThreads();
    scheduler.StopCamera();
    scheduler.StopWindow();

    return 0;
}