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
    /**
     * @brief s4_Upscale is a upscaler that allows multiple types of upscaling and image manip operations.
     * 
     */
    s4_Upscale();

    /**
     * @brief s4_Upscale is a upscaler that allows multiple types of upscaling and image manip operations.
     * 
     * @param m_h  Size to upscale to
     * @param m_w  Size to upscale to
     * @param mode Modes that can be selected from s4_Upscale_Modes. Use OR operation to select multiple modes. 
     */
    s4_Upscale(int m_h, int m_w, unsigned=static_cast<unsigned>(NEAREST | BINARY | NORMALIZE));
    ~s4_Upscale();

    /**
     * @brief Calls the upscale operation
     * 
     */
    torch::Tensor operator()(torch::Tensor &);

    /* You must call assign args after changing arguments */
    /**
     * @brief assign_args() must be called if the arguments to s4_Upscaler has changed, otherwise
     *        the changes might not take place.
     * 
     */
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
