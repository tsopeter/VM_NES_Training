#ifndef s4_pencoder_hpp__
#define s4_pencoder_hpp__

#include <torch/torch.h>
#include <raylib.h>

#include "../s2/quantize.hpp"
#include "../s3/cam.hpp" /* u8Image */

#if defined(__linux__)
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#endif
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
using cudaGraphicsResource=int;
#endif




/**
 * @brief PEncoder is used to generate variable-sized binary mask for PLM.
 * 
 */
class PEncoder {
public:
    /**
     * @brief PEncoder is used to generate variable-sized binary masks for PLM.
     *        If you need to simply generate a Image from Tensor, use s4_Utils::TensorToImage.
     * 
     */
    PEncoder ();
    PEncoder (int x, int y, int h, int w);
    ~PEncoder();

    /* Default methods for encoding torchTensors to other types */

    /**
     * @brief Encode_u8Image encodes a 8-bit torch tensor to u8Image, as grayscale.
     *        u8Image definition can be seen in /s3/cam.hpp
     * 
     * 
     */
    u8Image       Encode_u8Image (torch::Tensor &x);

    /**
     * @brief Encode_u8Tensor encodes 8-bit torch tensor to 1-bit torch Tensor
     * 
     * 
     */
    torch::Tensor Encode_u8Tensor(torch::Tensor &x);

    /**
     * @brief Encode_Image encodes 8-bit torch tensor to 8-bit grayscale image
     *        This is just an alias for u8Tensor_Image. It however, matches naming Encode_<format>
     *        used elsewhere.
     * 
     */
    Image         Encode_Image   (torch::Tensor &x);

    /**
     * @brief MEncode_u8Tensor encodes multiple 8-bit torch tensors to a single 32-bit torch tensor.
     *        There is a limit of 24 tensors at a time, due to limitations of the PLM.
     * 
     */
    torch::Tensor MEncode_u8Tensor (torch::Tensor &x);

    /**
     * @brief MEncode_u8Tensor2 encodes multiple 8-bit torch tensors to a single 32-bit torch tensor.
     *        There is a limit of 24 tensors at a time, due to limitations of the PLM.
     *        
     *        The main difference between MEncode_u8Tensor and MEnocde_u8Tensor2 (this one), is
     *        that MEncode_u8Tensor2 expects it to be quantized beforehand.
     */
    torch::Tensor MEncode_u8Tensor2 (torch::Tensor &x);

    /**
     * @brief MEncode_u8Tensor2 encodes multiple 8-bit torch tensors to a single 32-bit torch tensor.
     *        There is a limit of 24 tensors at a time, due to limitations of the PLM.
     *        
     *        The main difference between MEncode_u8Tensor and MEnocde_u8Tensor2 (this one), is
     *        that MEncode_u8Tensor2 expects it to be quantized beforehand.
     */
    torch::Tensor MEncode_u8Tensor2 (const torch::Tensor &x);
    
    // High resolution encoding based on upscaling using GPU
    torch::Tensor MEncode_u8Tensor3 (const torch::Tensor &x);

    // Lower resolution encoding based on bit-manip
    torch::Tensor MEncode_u8Tensor4 (const torch::Tensor &x);

    // Tile-based encoding, requires a custom-shader
    torch::Tensor MEncode_u8Tensor5 (const torch::Tensor &x);

    torch::Tensor MEncode_u8Tensor_Categorical (const torch::Tensor &x);

    torch::Tensor MEncode_u8Tensor_Binary (const torch::Tensor &x);


    int m_x, m_y; // Used for placing the phase mask on (x,y), assuming that the phase
                  // mask is smaller than the image plane
    int m_h, m_w;

    /**
     * @brief Takes in a phase tensor of size [n, H, W] and returns it as an Image encoded in 24-bits
     * 
     */
    Image         u8Tensor_Image (torch::Tensor &x);

    /**
     * @brief Takes in a modified image, and returns as whatever shape it was originally as 24-bit image.
     *        Modification must be done via Encode_u8Tensor or MEncode_u8Tensor
     */
    Image         u8MTensor_Image (torch::Tensor &x);

    /**
     * @brief Takes in a modified image, and returns as whatevery shape it was originally as 24-bit Texture using
     *        OpenGL. Modification must be done via Encode_u8Tensor or MEncode_u8Tensor
     */
    Texture       u8Tensor_Texture (torch::Tensor &x);

    /**
     * @brief Takes in a modified image, and returns as whatevery shape it was originally as 24-bit Texture using
     *        CPU-only operations. Modification must be done via Encode_u8Tensor or MEncode_u8Tensor
     */
    Texture       u8Tensor_Texture_CPU (torch::Tensor &x);

    /**
     * @brief Takes in images stored as [n, H/2, W/2] and maps it to each phase candidate
     * 
     */
    torch::Tensor ImageTensorMap (torch::Tensor &x1, torch::Tensor &i0);

    /**
     * @brief BImageTensorMap broadcasts a single image in i0 to n phase candidates
     * 
     */
    torch::Tensor BImageTensorMap (torch::Tensor &x1, torch::Tensor &i0);

    torch::Tensor upscale_ (const torch::Tensor &x, int scale_h, int scale_w);

    void init_pbo ();

private:

    /* Quantize is used for mapping float32 to 4-bit pseudo-quantized */
    Quantize q;

    /* Masks determine the actual mapping from 4-bit depth to 4-bit spatial, used by PLM */
    torch::Tensor masks;

    void validate_args();

    // Precomputed bit shifts for 24-channel vectorized encoding (used in MEncode_u8Tensor2)
    torch::Tensor shifts;

    // OpenGL/CUDA interop members
    GLuint m_textureID;
    GLuint m_pbo;
    cudaGraphicsResource* m_cuda_pbo_resource;
    bool m_texture_initialized;

};

#endif
