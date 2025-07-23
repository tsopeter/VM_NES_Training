#include "e20.hpp"
#include "../s2/von_mises.hpp"
#include "../s4/model.hpp"
#include "../s4/optimizer.hpp"
#include "../s3/window.hpp"

#include <iostream>
#include <torch/torch.h>
#include <complex>

/**
 * e20 is used to investigate why e19
 * is not *currently* working
 */

class e20_Model : public s4_Model {
public:
    e20_Model (int64_t Height, int64_t Width, int64_t n_samples, float std=0.5)
    : m_Height(Height), m_Width(Width), m_Samples(n_samples), m_kappa(1.0f/std) {

        m_parameter = torch::rand({Height, Width}).to(DEVICE);
        m_parameter.set_requires_grad(true);

        m_dist.set_mu (m_parameter, m_kappa);
    }

    ~e20_Model () override {}

    torch::Tensor sample () {
        torch::NoGradGuard no_grad;
        m_action = m_dist.sample(m_Samples);
        return m_action;
    }

    torch::Tensor logp_action () override {
        return m_dist.log_prob(m_action);
    }

    int64_t N_samples () const override {
        return m_Samples;
    }

    std::vector<torch::Tensor> parameters () {
        return {m_parameter};
    }

    torch::Tensor &get_parameters () {
        return m_parameter;
    }

    Texture TensorToTexture (torch::Tensor &t, float clamp_limit=0.01f) {
        // Assumes t is a 2D tensor [H, W]
        torch::Tensor t_cpu = t.detach().to(torch::kCPU).contiguous();

        int height = t_cpu.size(0);
        int width = t_cpu.size(1);

        // Clamp to [0, 1] and scale to 0–255, then convert to uint8
        
        /* Re orientate */
        t_cpu = t_cpu - t_cpu.min();

        if (clamp_limit <= 0) {
            t_cpu = t_cpu / t_cpu.max();
        }
        else {
            t_cpu = t_cpu.clamp(0.0, clamp_limit) / clamp_limit;
        }
        t_cpu = (t_cpu * 255).round().to(torch::kUInt8).contiguous();

        Image image = {
            .data = (void*)t_cpu.data_ptr(),
            .width = width,
            .height = height,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
        };

        Texture tex = LoadTextureFromImage(image);
        return tex;
    }

private:
    int64_t  m_Height, m_Width;
    int64_t  m_Samples;
    VonMises m_dist;

    float m_kappa = 1.0f/0.01f;

    torch::Tensor m_parameter;
    torch::Tensor m_action;
};

torch::Tensor env (torch::Tensor p, double power=1.0f) {
    torch::NoGradGuard no_grad;

    p = p.unsqueeze(1); // {N_samples, 1, H, W}

    // Extract dimensions from p
    int64_t N_samples = p.size(0);
    int64_t H = p.size(2);
    int64_t W = p.size(3);

    // For each sample apply
    // exp(1j * p);
    p = torch::polar(torch::ones_like(p) * power, p);

    // ifftshift(ifft2(p), (-2, -1))
    p = torch::fft::ifftshift(torch::fft::ifft2(p), {-2, -1});
    p = p.abs().pow(2);

    int64_t block_width = W / 8;
    int64_t block_height = H / 8;
    torch::Tensor class_1 = torch::zeros({H, W}, torch::kFloat32).to(p.device());
    class_1.slice(0, 0, block_height).slice(1, 0, block_width).fill_(1.0);
    class_1 = class_1.unsqueeze(0).expand({N_samples, H, W});
    class_1 = class_1.unsqueeze(1);

    torch::Tensor u1 = (p * class_1).sum({1, 2, 3});    // [N_samples]
    torch::Tensor u2 = (p * (1 - class_1)).sum({1, 2, 3}); // [N_samples]

    torch::Tensor logits = torch::stack({u1, u2}, 1);  // [N_samples, 2]
    torch::Tensor targets = torch::zeros({N_samples}, torch::kLong).to(p.device());  // class 1 as target
    torch::Tensor loss = torch::nn::functional::cross_entropy(
        logits,
        targets,
        torch::nn::functional::CrossEntropyFuncOptions().reduction(torch::kNone)
    );
    return loss;
}


int e20 () {
    int64_t N_samples = 128;
    int64_t N_dim = 128;

    e20_Model model {N_dim, N_dim, N_samples, 0.3};

    s3_Window window;
    window.Height = 1024;
    window.Width  = 1024;
    window.wmode  = WINDOWED;
    window.fmode  = NO_TARGET_FPS;
    window.monitor = 0;
    window.load();

    torch::optim::Adam adam (
        model.parameters (),
        torch::optim::AdamOptions(0.1)
    );

    s4_Optimizer opt {
        adam, model
    };

    while (!WindowShouldClose()) {
        auto samples = model.sample (); // [N_samples, N_dim, N_dim]
        auto rewards = -env(samples, 25.f);   // [N_samples]
        auto average_rewards = rewards.mean().item<double>();
        std::string reward_text = "Avg Reward: " + std::to_string(average_rewards);
        opt.step(rewards);

        torch::NoGradGuard no_grad;
        auto params  = model.get_parameters();
        params = torch::fft::ifftshift(torch::fft::ifft2(params)).abs().pow(2);
        auto texture = model.TensorToTexture(params, 0.01);
        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro (texture, 
                {0, 0, static_cast<float>(N_dim), static_cast<float>(N_dim)}, 
                {0, 0, static_cast<float>(window.Width), static_cast<float>(window.Height)},
                 {0, 0}, 0.0f, WHITE);
            DrawFPS(10, 10);
            DrawText(reward_text.c_str(), 10, 30, 20, RAYWHITE);
        EndDrawing();
        UnloadTexture(texture);
    }

    



    return 0;
}