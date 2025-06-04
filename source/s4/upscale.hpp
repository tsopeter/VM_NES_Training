#ifndef s4_upscale_hpp__
#define s4_upscale_hpp__

#include <torch/torch.h>

enum s4_Upscale_Modes : unsigned {
    NEAREST   = 1,
    BINARY    = 2,
    BILINEAR  = 4,
    NORMALIZE = 8,
    ROUND     = 16,
};

class s4_Upscale {
public:
    s4_Upscale();
    s4_Upscale(int m_h, int m_w, unsigned=static_cast<unsigned>(NEAREST | BINARY | NORMALIZE));
    ~s4_Upscale();

    torch::Tensor operator()(torch::Tensor &);

    /* You must call assign args after changing arguments */
    void assign_args();

    int m_h, m_w;
    unsigned m_mode;

private:

    torch::Tensor upscale(torch::Tensor &x);
    torch::Tensor binarize(torch::Tensor &x);
    torch::Tensor normalize(torch::Tensor &x);
    torch::Tensor round(torch::Tensor &x);

    bool bin;
    bool norm;
    bool rnd;
    torch::nn::functional::InterpolateFuncOptions::mode_t p_mode;

    void validate_args();
};

#endif
