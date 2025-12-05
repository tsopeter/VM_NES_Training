#pragma once
#include <torch/torch.h>
#include <numbers>
#include <functional>
#include <cmath>

#include "dist.hpp"

class VonMises : public Dist {
public:
    /**
     * @brief VonMises (or circular normal) is implemented
     *        using
     *        
     *        f(x | u, k) = exp(k * cos (x - u))/ (2 * pi * I0(k))
     * 
     *        where
     * 
     *        I0(k) is the first order Bessel function.
     * 
     *        This function is based off of implementation in PyTorch
     *        library.
     * 
     *        See https://github.com/pytorch/pytorch/blob/main/torch/distributions/von_mises.py
     * 
     *        It has been streamlined to accept only large kappas (or small variance).
     * 
     *        Code snippet:
     * 
     *          auto dist    = VonMises(mu, kappa);
     *          auto samples = dist.sample(N);
     * 
     */
    VonMises();
    VonMises(torch::Tensor &mu, torch::Tensor &kappa);
    VonMises(torch::Tensor &mu, double kappa);
    ~VonMises();

    /**
     * @brief Samples from VonMises distribution. Uses no gradient.
     * 
     * @param int Number of samples. It always samples such that
     *             mu = [X,Y,...], sample(N) = [N,X,Y,...]
     * @return Sampled result as torch Tensor
     */
    torch::Tensor sample(int) override;

    /**
     * @brief Computes the Log Probability given of a tensor
     * 
     */
    torch::Tensor log_prob(torch::Tensor&) override;

    /**
     * @brief Privately used rejection filter for Von Mises
     *        Note: It's publically accessible, but it's not really to be used
     *        anywhere else other than some private functions
     */
    torch::Tensor m_rejection_r ();

    /**
     * @brief Prints stats
     * 
     */
    void print_stats () const;

    /**
     * @brief Set mu
     * 
     */
    void set_mu (torch::Tensor &mu, double kappa);

    /**
     * @brief Set std
     */
    void set_std (double std);
    double get_std ();
    
    torch::Tensor m_mu, m_kappa;
    torch::Tensor m_r;
    double std;
    double logp = std::log(2.0 * M_PI);  // using M_PI from cmath

};