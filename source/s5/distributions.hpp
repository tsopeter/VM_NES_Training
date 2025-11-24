#ifndef distributions_hpp__
#define distributions_hpp__

#include <torch/torch.h>
#include "../s2/dist.hpp"

namespace Distributions {

class Definition {
public:
    Definition () {}
    virtual ~Definition () {}
    virtual torch::Tensor sample (int n) = 0;
    virtual torch::Tensor log_prob(torch::Tensor &t) = 0;
    virtual torch::Tensor base(int n) = 0;
    virtual std::string get_name() const = 0;
    virtual torch::Tensor entropy() = 0;
    virtual torch::Tensor entropy_no_grad() = 0;
    virtual torch::Tensor probs() = 0;
};

class Normal : public Definition {
public:
    Normal (torch::Tensor &mu, double std);
    ~Normal ();
    torch::Tensor sample (int n) override;
    torch::Tensor base(int n) override;
    torch::Tensor log_prob(torch::Tensor &t) override;
    std::string get_name() const override;
    torch::Tensor entropy() override;
    torch::Tensor entropy_no_grad() override;
    torch::Tensor probs() override;
private:
    torch::Tensor m_mu;
    double m_std;
};

class xNES_Normal : public Definition {
public:
    xNES_Normal (torch::Tensor &mu, torch::Tensor &cov);
    ~xNES_Normal ();
    torch::Tensor sample (int n) override;
    torch::Tensor base(int n) override;
    torch::Tensor log_prob(torch::Tensor &t) override;
    std::string get_name() const override;
    torch::Tensor entropy() override;
    torch::Tensor entropy_no_grad() override;
    torch::Tensor probs() override;
private:
    torch::Tensor m_mu;
    torch::Tensor m_cov;
};

class Categorical : public Definition {
public:
    Categorical (torch::Tensor &logits);
    ~Categorical ();
    torch::Tensor sample (int n) override;
    torch::Tensor base (int n) override;
    torch::Tensor log_prob(torch::Tensor &t) override;
    std::string get_name() const override;
    torch::Tensor entropy() override;
    torch::Tensor entropy_no_grad() override;
    torch::Tensor probs() override;
private:
    torch::Tensor m_logits;

    torch::Tensor logits_to_probs (torch::Tensor&);

};

class Bernoulli : public Definition {
public:
    Bernoulli (torch::Tensor &logits);
    ~Bernoulli ();
    torch::Tensor sample (int n) override;
    torch::Tensor base (int n) override;
    torch::Tensor log_prob(torch::Tensor &t) override;
    std::string get_name() const override;
    torch::Tensor entropy() override;
    torch::Tensor entropy_no_grad() override;
    torch::Tensor probs() override;
private:
    torch::Tensor m_logits;

    torch::Tensor logits_to_probs ();

};

class Binary : public Definition {
public:
    Binary (torch::Tensor &logits);
    ~Binary ();
    torch::Tensor sample (int n) override;
    torch::Tensor base (int n) override;
    torch::Tensor log_prob(torch::Tensor &t) override;
    std::string get_name() const override;
    torch::Tensor entropy() override;
    torch::Tensor entropy_no_grad() override;
    torch::Tensor probs() override;
private:
    Categorical *dist = nullptr;
};

}

#endif
