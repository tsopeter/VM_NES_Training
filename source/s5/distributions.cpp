#include "distributions.hpp"
#include <numeric>
#include "tutils.hpp"

Distributions::Normal::Normal (torch::Tensor &mu, double std) {
    m_mu = mu;
    m_std = std;
}

Distributions::Normal::~Normal () {

}

std::string Distributions::Normal::get_name() const {
    return "normal";
}

torch::Tensor Distributions::Normal::sample (int n) {
    torch::NoGradGuard no_grad;
    std::cout << "INFO: [Distributions::Normal::sample] Sampling " << n << " samples from Normal distribution with mean shape " << m_mu.sizes() << " and std " << m_std << ".\n";
    
    // Broadcast m_mu to match the sample shape
    auto mu_shape = m_mu.sizes();
    auto sample_shape = torch::IntArrayRef({n}).vec();
    sample_shape.insert(sample_shape.end(), mu_shape.begin(), mu_shape.end());
    torch::Tensor eps = torch::randn(sample_shape, m_mu.options());
    return (m_mu.unsqueeze(0).expand_as(eps) + m_std * eps).contiguous();
}

torch::Tensor Distributions::Normal::base(int n) {
    torch::NoGradGuard no_grad;
    
    // Broadcast m_mu to match the sample shape
    auto mu_shape = m_mu.sizes();
    auto sample_shape = torch::IntArrayRef({n}).vec();
    sample_shape.insert(sample_shape.end(), mu_shape.begin(), mu_shape.end());
    return m_mu.unsqueeze(0).expand(sample_shape).contiguous();
}

torch::Tensor Distributions::Normal::log_prob(torch::Tensor &t) {
    // Compute log probability of t under Normal(m_mu, m_std)
    auto var = m_std * m_std;
    auto log_scale = std::log(m_std);

    return (
        -((t - m_mu).pow(2)) / (2 * var)
        - log_scale
        - std::log(2 * M_PI) / 2
    );
}

torch::Tensor Distributions::Normal::entropy() {
    // Entropy of Normal distribution: 0.5 * log(2 * pi * e * var)
    auto var = m_std * m_std;

    // Convert to tensor
    auto _entropy = 0.5 + 0.5 * std::log(2 * M_PI * var);
    return torch::tensor(_entropy, m_mu.options());
}

torch::Tensor Distributions::Normal::entropy_no_grad() {
    return entropy(); // note that entropy has no grad anyway since it only is determed by std
}

Distributions::Categorical::Categorical (torch::Tensor &logits) {
    m_logits = logits;
}

Distributions::Categorical::~Categorical () {

}

std::string Distributions::Categorical::get_name() const {
    return "categorical";
}

torch::Tensor Distributions::Categorical::sample (int n) {
    torch::NoGradGuard no_grad;
    
    // Convert logits to probabilities
    auto logits = m_logits - m_logits.logsumexp(-1, true);
    torch::Tensor probs = logits_to_probs(logits);
    
    // Get the number of events (categories)
    int num_events = logits.size(-1);
    
    // Reshape probabilities to 2D for multinomial sampling
    auto original_shape = probs.sizes();
    torch::Tensor probs_2d = probs.reshape({-1, num_events});
    
    // Sample using multinomial
    torch::Tensor samples_2d = torch::multinomial(probs_2d, n, true).transpose(0, 1);
    
    // Reshape back to the desired shape
    std::vector<int64_t> sample_shape = {n};
    for (int64_t dim : original_shape.slice(0, original_shape.size() - 1)) {
        sample_shape.push_back(dim);
    }
    
    return samples_2d.reshape(sample_shape).contiguous();
}

// Actually returns the category with the highest probability
torch::Tensor Distributions::Categorical::base(int n) {
    torch::NoGradGuard no_grad;

    // Convert logits to normalized log-probabilities
    auto logits = m_logits - m_logits.logsumexp(-1, true); // [*, num_events]
    std::cout << "INFO: [Distributions::Categorical::base] Logits shape: " << logits.sizes() << '\n';

    // Convert logits to probabilities
    torch::Tensor probs = logits_to_probs(logits); // [*, num_events]

    // Get the index of the maximum probability along the last dimension
    torch::Tensor max_indices = std::get<1>(probs.max(-1, true)); // [*, 1]

    // Reshape and expand: [H, W, 1] → [H, W] → [n, H, W]
    torch::Tensor expanded =
        max_indices.squeeze(-1)     // remove last dimension -> [H, W]
                   .unsqueeze(0)    // add batch dimension -> [1, H, W]
                   .expand({n, -1, -1})  // repeat along n
                   .contiguous();

    return expanded;
}

torch::Tensor Distributions::Categorical::log_prob(torch::Tensor &t) {
    // Convert input to long tensor and add dimension for gathering
    torch::Tensor value = t.to(torch::kLong).unsqueeze(-1);
    
    // Broadcast value and logits to same shape
    auto logits = m_logits - m_logits.logsumexp(-1, true);
    auto broadcasted = torch::broadcast_tensors({value, logits});
    value = broadcasted[0];
    torch::Tensor log_pmf = broadcasted[1];
    
    // Take only the first element along the last dimension for value
    value = value.slice(-1, 0, 1);
    
    // Gather log probabilities and squeeze the last dimension
    return log_pmf.gather(-1, value).squeeze(-1);
}

torch::Tensor Distributions::Categorical::entropy() {
    // Convert logits to normalized log-probabilities
    auto _logits = m_logits - m_logits.logsumexp(-1, true);
    auto min_value = tutils::min_value(_logits);
    auto logits = torch::clamp(_logits, min_value);

    auto probs = logits_to_probs(_logits);
    auto p_log_p = logits * probs;

    return -p_log_p.sum(-1);
}

torch::Tensor Distributions::Categorical::entropy_no_grad() {
    auto detached_logits = m_logits.detach();
    // Convert logits to normalized log-probabilities
    auto _logits = detached_logits - detached_logits.logsumexp(-1, true);

    // Assume that min_value is a double (should be fine for all dtypes)
    auto min_value = tutils::min_value(_logits);


    auto logits = torch::clamp(_logits, min_value);

    auto probs = logits_to_probs(_logits);
    auto p_log_p = logits * probs;

    return -p_log_p.sum(-1);
}

torch::Tensor Distributions::Categorical::logits_to_probs (torch::Tensor &t) {
    // Compute softmax
    return torch::nn::functional::softmax(t, -1);
}

Distributions::Bernoulli::Bernoulli (torch::Tensor &logits) {
    m_logits = logits;
}

Distributions::Bernoulli::~Bernoulli () {

}

std::string Distributions::Bernoulli::get_name() const {
    return "bernoulli";
}

torch::Tensor Distributions::Bernoulli::sample (int n) {
    torch::NoGradGuard no_grad;
    
    // Convert logits to probabilities
    torch::Tensor probs = logits_to_probs();

    auto expanded_shape = probs.sizes().vec();
    expanded_shape.insert(expanded_shape.begin(), n);
    probs = probs.expand(expanded_shape);

    // Use bernoulli sampling
    return torch::bernoulli(probs).contiguous();
}

// This should actually return [0, 1] based on whether p > 0.5
torch::Tensor Distributions::Bernoulli::base(int n) {
    torch::NoGradGuard no_grad;
    
    // Convert logits to probabilities
    torch::Tensor probs = logits_to_probs();
    
    // Return 1 if p > 0.5, else 0 (this is the mode of the Bernoulli distribution)
    torch::Tensor mode = (probs > 0.5).to(probs.dtype());
    
    // Expand to match sample shape [n, ...]
    auto expanded_shape = mode.sizes().vec();
    expanded_shape.insert(expanded_shape.begin(), n);
    
    return mode.expand(expanded_shape).contiguous();
}

torch::Tensor Distributions::Bernoulli::log_prob(torch::Tensor &t) {
    // Broadcast logits and value to same shape
    auto broadcasted = torch::broadcast_tensors({m_logits, t});
    torch::Tensor logits = broadcasted[0];
    torch::Tensor value = broadcasted[1];
    
    // Compute -binary_cross_entropy_with_logits(logits, value, reduction="none")
    // BCE with logits formula: max(x, 0) - x * z + log(1 + exp(-abs(x)))
    // where x = logits, z = value
    torch::Tensor max_val = torch::relu(logits);
    torch::Tensor loss = max_val - logits * value + torch::log1p(torch::exp(-torch::abs(logits)));
    
    return -loss;
}

torch::Tensor Distributions::Bernoulli::logits_to_probs () {
    // Compute sigmoid
    return torch::sigmoid(m_logits);
}

torch::Tensor Distributions::Bernoulli::entropy() {
    // Positive binary cross entropy with logits
    auto _logits = m_logits;
    auto _probs  = torch::sigmoid(_logits);

    auto vals = torch::broadcast_tensors({_logits, _probs});
    auto logits = vals[0];
    auto probs  = vals[1];
    torch::Tensor max_val = torch::relu(logits);
    torch::Tensor loss = max_val - logits * probs + torch::log1p(torch::exp(-torch::abs(logits)));

    return loss;
}

torch::Tensor Distributions::Bernoulli::entropy_no_grad() {
    // Positive binary cross entropy with logits
    auto _logits = m_logits.detach();
    auto _probs  = torch::sigmoid(_logits);

    auto vals = torch::broadcast_tensors({_logits, _probs});
    auto logits = vals[0];
    auto probs  = vals[1];
    torch::Tensor max_val = torch::relu(logits);
    torch::Tensor loss = max_val - logits * probs + torch::log1p(torch::exp(-torch::abs(logits)));

    return loss;
}

Distributions::Binary::Binary (torch::Tensor &logits) {
    // Ensure that logits are only two classes (i.e., last dimension is 2)
    if (logits.size(-1) != 2) {
        throw std::runtime_error("Distributions::Binary: logits must have last dimension of size 2.\n");
    }
    dist = new Categorical (logits);
}

Distributions::Binary::~Binary () {
    delete dist;
}

std::string Distributions::Binary::get_name() const {
    return "binary";
}

torch::Tensor Distributions::Binary::sample (int n) {
    return dist->sample(n);
}

torch::Tensor Distributions::Binary::base(int n) {
    return dist->base(n);
}

torch::Tensor Distributions::Binary::log_prob(torch::Tensor &t) {
    return dist->log_prob(t);
}

torch::Tensor Distributions::Binary::entropy() {
    return dist->entropy();
}

torch::Tensor Distributions::Binary::entropy_no_grad() {
    return dist->entropy_no_grad();
}