#include "upscale.hpp"
#include <stdexcept>

s4_Upscale::s4_Upscale()
: m_h(0), m_w(0), m_mode(0) {

}

s4_Upscale::s4_Upscale(int h, int w, unsigned mode) :
m_h(h), m_w(w), m_mode(mode) {
    assign_args();
}

s4_Upscale::~s4_Upscale() {

}

torch::Tensor s4_Upscale::upscale(torch::Tensor &x) {
    // Expecting input of shape [n, H, W]
    // Reshape to [n, 1, H, W] for interpolate
    auto x_reshaped = x.unsqueeze(1);

    // Interpolate to target size [m_h, m_w]
    auto out = torch::nn::functional::interpolate(
        x_reshaped,
        torch::nn::functional::InterpolateFuncOptions()
            .size(std::vector<int64_t>({m_h, m_w}))
            .mode(p_mode)
    );

    // Remove the channel dimension -> shape [n, m_h, m_w]
    return out.squeeze(1);
}
torch::Tensor s4_Upscale::binarize(torch::Tensor &x) {
    return (bin) ? torch::where(x > 0, 1, 0).to(x.dtype()) : x;
}

torch::Tensor s4_Upscale::normalize(torch::Tensor &x) {
    return (norm) ? x / 255 : x;
}

torch::Tensor s4_Upscale::round(torch::Tensor &x) {
    return (rnd) ? torch::round(x) : x;
}

torch::Tensor s4_Upscale::operator()(torch::Tensor &x) {
    auto x2 = normalize(x);
    auto x3 = upscale(x2);
    auto x4 = round(x3);
    return binarize(x4);
}

void s4_Upscale::validate_args () {
    if (m_h == 0 || m_w == 0)
        throw std::runtime_error("s4_Upscale::validate_args: Args are not valid");
}

void s4_Upscale::assign_args () {
    /* default */
    p_mode = torch::kNearest;

    if ((m_mode & NEAREST)  != 0) p_mode = torch::kNearest;
    if ((m_mode & BILINEAR) != 0) p_mode = torch::kBilinear;
    
    bin  = ((m_mode & BINARY) != 0);
    norm = ((m_mode & NORMALIZE) != 0);
    rnd  = ((m_mode & ROUND) != 0);
}