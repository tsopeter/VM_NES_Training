#include "e2.hpp"
#include "../s2/dist.hpp"       /* Distribution Virtual Class*/
#include "../s4/model.hpp"      /* Model Virtual Class  */
#include "../s4/optimizer.hpp"  /* Black-Box Optimizer */
#include "../s2/von_mises.hpp"  /* Von Mises distribution */

#include <iostream>
#include <torch/torch.h>

//#define E2_USE_NORMAL_

class e2_Normal : public Dist {
public:
    e2_Normal ()
    {}

    e2_Normal (torch::Tensor &mu, double std)
    : m_mu(mu), m_std(std)
    {}

    ~e2_Normal () override {}

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

    torch::Tensor m_mu;
    double m_std;

};

class e2_Model : public s4_Model {
public:
    e2_Model (int64_t m, int64_t n) :
    m_m(m), m_n(n) 
    {
        m_parameter = torch::rand({m}).to(DEVICE);
        m_parameter.set_requires_grad(true);

#if defined(E2_USE_NORMAL_)
        m_normal.m_mu  = m_parameter;
        m_normal.m_std = 0.05f;
#else
        m_normal.set_mu(m_parameter, 1.0f/0.05f);
        //m_normal.m_mu = m_parameter;
        //m_normal.m_kappa = torch::ones_like(m_parameter) * (1.0f/0.05f);
        //m_normal.m_r = m_normal.m_rejection_r ();
#endif
    }

    ~e2_Model () override {

    }

    torch::Tensor sample () {
        torch::NoGradGuard no_grad;
        m_action = m_normal.sample(m_n);
        return m_action;
    }

    torch::Tensor logp_action () override {
        return m_normal.log_prob(m_action);
    }

    int64_t N_samples () const override {
        return m_n;
    }

    std::vector<torch::Tensor> parameters () {
        return {m_parameter};
    }

    torch::Tensor &get_parameters () {
        return m_parameter;
    }

private:
    int64_t m_m;
    int64_t m_n;

    torch::Tensor m_parameter;
    torch::Tensor m_action;
#if defined(E2_USE_NORMAL_)
    e2_Normal m_normal {};
#else
    VonMises m_normal {};
#endif

};

torch::Tensor env (torch::Tensor &p) {
    // MSE
    torch::NoGradGuard no_grad;
    auto d = (p - 0.2f);
    return torch::sum(
        d * d
    ,1);    /*dim=1*/
}

int e2 () {
    int64_t m = 5;
    e2_Model model {m, 32};

    /* torch optimizer */
    torch::optim::Adam adam(model.parameters(), torch::optim::AdamOptions(0.1));

    /* Black-Box optimizer */
    s4_Optimizer opt {
        adam, model
    };

    /* Training loop */
    for (int i = 0; i < 200; ++i) {
        auto samples = model.sample();  // [N, C]
        auto rewards = -env(samples);   // [N]

        opt.step(rewards);
    }

    /* Print out parameters */
    std::cout<<model.get_parameters()<<'\n';


    return 0;
}