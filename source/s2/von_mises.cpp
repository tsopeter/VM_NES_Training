#include "von_mises.hpp"
#include <array>
/**
 *  Helper functions
 */

static double vm_sum_rounds = 0, vm_min_rounds = 10000000, vm_max_rounds = -10000000;
static int vm_calls = 0;

/**
 * Modified Bessel Function based on
 * PyTorch implmentation
 * 
 * See: https://github.com/pytorch/pytorch/blob/main/torch/distributions/von_mises.py
 * 
 * Returns ``log(I_0(x))`` for ``x > 0``
 * 
 */

using rejection_fn = std::function<torch::Tensor>;

#define VONMISES_PRECISION torch::kFloat32

std::vector<double>
 _I0_COEF_SMALL = {
    1.0,
    3.5156229,
    3.0899424,
    1.2067492,
    0.2659732,
    0.360768e-1,
    0.45813e-2,
 };

std::vector<double>
_I0_COEF_LARGE = {
    0.39894228,
    0.1328592e-1,
    0.225319e-2,
    -0.157565e-2,
    0.916281e-2,
    -0.2057706e-1,
    0.2635537e-1,
    -0.1647633e-1,
    0.392377e-2,
};

torch::Tensor m_eval_poly(torch::Tensor &y, std::vector<double> &coef) {
    // Assume coef.size() > 0
    auto result = torch::full_like(y, coef.back());  // Start with the last coefficient
    for (int i = coef.size() - 2; i >= 0; --i) {
        result = coef[i] + y * result;
    }
    return result;
}

torch::Tensor m_log_modified_bessel_fn_0 (
    torch::Tensor &x
) {
    auto y = x / 3.75;
    y      = y * y;

    auto small = m_eval_poly(y, _I0_COEF_SMALL);

    y = 3.75 / x;
    auto large = x - 0.5 * x.log() + m_eval_poly(y, _I0_COEF_LARGE);

    auto result = torch::where(x < 3.75, small, large);
    return result;
}

torch::Tensor m_rejection_sampling(
    const torch::Tensor &loc,
    const torch::Tensor &concentration,
    const torch::Tensor &proposal_r,
    torch::Tensor x  // in-place updated
) {
    auto done = torch::zeros_like(x, torch::kBool).to(x.device());
    const auto pi = M_PI;
    ++vm_calls;

    int rounds = 0;
    while (!done.all().item<bool>()) {
        auto x_shape = x.sizes();
        std::vector<int64_t> shape{3};
        shape.insert(shape.end(), x_shape.begin(), x_shape.end());
        auto u = torch::rand(shape, loc.options().device(x.device()));
        auto u1 = u[0];
        auto u2 = u[1];
        auto u3 = u[2];

        auto z = torch::cos(pi * u1);
        auto f = (1 + proposal_r * z) / (proposal_r + z);
        auto c = concentration * (proposal_r - f);

        auto accept_1 = (c * (2 - c) - u2) > 0;
        auto accept_2 = ((c / u2).log() + 1 - c) >= 0;
        auto accept = accept_1 | accept_2;

        if (accept.any().item<bool>()) {
            auto theta = (u3 - 0.5).sign() * f.acos();
            x = torch::where(accept, theta, x);
            done = done | accept;
        }
        ++rounds;
    }

    if (rounds < vm_min_rounds)
        vm_min_rounds = rounds;

    if (rounds > vm_max_rounds)
        vm_max_rounds = rounds;

    vm_sum_rounds += rounds;

    return torch::remainder(x + pi + loc, 2 * pi) - pi;
}

VonMises::VonMises ()
{}

VonMises::VonMises (torch::Tensor &mu, torch::Tensor &kappa) :
    m_mu(mu), m_kappa(kappa) {

    m_mu    = m_mu.to(VONMISES_PRECISION);
    m_kappa = m_kappa.to(VONMISES_PRECISION).to(m_mu.device());
    m_r     = m_rejection_r();  /* precomputed */
}

VonMises::VonMises (torch::Tensor &mu, double kappa) :
    m_mu(mu) {
    m_kappa = torch::full_like(mu, kappa, VONMISES_PRECISION).to(m_mu.device());
    m_r     = m_rejection_r();  /* precomputed */
}

VonMises::~VonMises () {

}

torch::Tensor VonMises::sample(int n) {
    torch::NoGradGuard no_grad;
    
    // Create sample shape
    auto shape = m_mu.sizes().vec();
    shape.insert(shape.begin(), n);

    // Allocate x
    auto x = torch::empty(shape, m_mu.options().dtype(VONMISES_PRECISION));

    auto result = m_rejection_sampling(m_mu, m_kappa, m_r, x);

    return result.to(m_mu.dtype());
}

torch::Tensor VonMises::log_prob (torch::Tensor &x) {
    auto p_logpi = m_kappa * torch::cos(x - m_mu);
    p_logpi = p_logpi - logp - m_log_modified_bessel_fn_0(m_kappa);
    return p_logpi;
}

torch::Tensor VonMises::m_rejection_r () {
    auto kappa2 = m_kappa * m_kappa;
    auto tau = 1 + (1 + 4 * kappa2).sqrt();
    auto rho = (tau - (2 * tau).sqrt()) / (2 * m_kappa);
    auto prop_r = (1 + (rho * rho)) / (2 * rho);
    return prop_r;
}

void VonMises::print_stats () const {
    std::cout<<"[Von Mises] Statistics\n"
               "\tm_rejection_sampling\n"
               "\t\tTotal Rounds: "<<vm_sum_rounds<<'\n'<<
               "\t\tAverage Rounds: "<<vm_sum_rounds/vm_calls<<'\n'<<
               "\t\tMin Rounds: "<<vm_min_rounds<<'\n'<<
               "\t\tMax Rounds: "<<vm_max_rounds<<'\n'<<
               "\t\tTotal calls: "<<vm_calls<<'\n';
}

// Output shape: [n, H, W]
/**
 *  Below is a copy of the VMF function that was used as reference to implement sample_muller(...)
 * 
 * 
    import numpy as np

    def random_vMF(mu, kappa, size=None):
        """
        Sample from the von Mises–Fisher (vMF) distribution with given mean direction `mu` and concentration `kappa`.
        Based on: https://hal.science/hal-04004568
        """
        # Determine number of samples and output shape
        n = 1 if size is None else np.prod(size)
        shape = () if size is None else tuple(np.ravel(size))

        # Normalize mean direction
        mu = np.asarray(mu)
        mu = mu / np.linalg.norm(mu)
        (d,) = mu.shape

        # Sample points orthogonal to mu
        z = np.random.normal(0, 1, (n, d))
        z /= np.linalg.norm(z, axis=1, keepdims=True)
        z = z - (z @ mu[:, None]) * mu[None, :]
        z /= np.linalg.norm(z, axis=1, keepdims=True)

        # Sample cos(θ) values
        cos = _random_vMF_cos(d, kappa, n)
        sin = np.sqrt(1 - cos ** 2)

        # Combine radial and angular components
        x = z * sin[:, None] + cos[:, None] * mu[None, :]
        return x.reshape((*shape, d))

    def _random_vMF_cos(d, kappa, n):
        """
        Rejection sampling to generate cos(θ) values with density:
        p(t) ∝ (1 - t^2)^((d-2)/2) * exp(kappa * t)
        """
        b = (d - 1) / (2 * kappa + np.sqrt(4 * kappa ** 2 + (d - 1) ** 2))
        x0 = (1 - b) / (1 + b)
        c = kappa * x0 + (d - 1) * np.log(1 - x0 ** 2)

        found = 0
        out = []
        while found < n:
            m = min(n, int((n - found) * 1.5))
            z = np.random.beta((d - 1) / 2, (d - 1) / 2, size=m)
            t = (1 - (1 + b) * z) / (1 - (1 - b) * z)
            test = kappa * t + (d - 1) * np.log(1 - x0 * t) - c
            accept = test >= -np.random.exponential(size=m)
            out.append(t[accept])
            found += len(out[-1])

        return np.concatenate(out)[:n]
 */
torch::Tensor VonMises::sample_muller(int n) {
    auto shape = m_mu.sizes().vec();
    int d = shape.size();
    shape.insert(shape.begin(), n);  // Result shape: [n, d]

    // Normalize mu
    torch::Tensor mu = m_mu / torch::linalg::norm(m_mu, -1, true);

    // Sample z ~ N(0,1) and make it orthogonal to mu
    auto z = torch::randn(shape, m_mu.options());
    auto dot = torch::sum(z * mu, -1, true);             // [n, 1]
    auto proj = dot * mu;                                // [n, d]
    z = z - proj;                                        // Make orthogonal
    z = z / torch::linalg::norm(z, -1, true);            // Normalize to unit vector

    // Sample cos(theta)
    int D = d;
    double kappa_val = m_kappa.item<double>();
    double b = (D - 1) / (2 * kappa_val + std::sqrt(4 * kappa_val * kappa_val + (D - 1) * (D - 1)));
    double x0 = (1 - b) / (1 + b);
    double c = kappa_val * x0 + (D - 1) * std::log(1 - x0 * x0);

    std::vector<double> cos_vals;
    cos_vals.reserve(n);

    while (cos_vals.size() < static_cast<size_t>(n)) {
        int m = std::min(n, static_cast<int>((n - cos_vals.size()) * 1.5));
        auto z_beta = torch::distributions::Beta((D - 1) / 2.0, (D - 1) / 2.0).sample({m});
        auto t = (1 - (1 + b) * z_beta) / (1 - (1 - b) * z_beta);
        auto test = kappa_val * t + (D - 1) * torch::log(1 - x0 * t) - c;
        auto accept = test >= -torch::rand({m}).exponential_();
        auto accepted = t.index({accept});
        cos_vals.insert(cos_vals.end(), accepted.data_ptr<double>(), accepted.data_ptr<double>() + accepted.numel());
    }

    auto cos_tensor = torch::from_blob(cos_vals.data(), {n}, m_mu.options()).clone();
    auto sin_tensor = torch::sqrt(1 - cos_tensor * cos_tensor).unsqueeze(1);  // [n,1]
    cos_tensor = cos_tensor.unsqueeze(1);                                     // [n,1]

    auto x = z * sin_tensor + mu * cos_tensor;
    return x.to(m_mu.dtype());
}