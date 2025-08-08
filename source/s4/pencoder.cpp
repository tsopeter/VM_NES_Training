#include "pencoder.hpp"
#include <iostream>
#include <cstdint>
#include <stdexcept>

#include "../utils/utils.hpp"
#include "../device.hpp"    /* Includes device macro */

/*
constexpr int32_t shifted_values[24] = 
{
    static_cast<int32_t>(1u << 31), static_cast<int32_t>(1u << 30),
    static_cast<int32_t>(1u << 29), static_cast<int32_t>(1u << 28),
    static_cast<int32_t>(1u << 27), static_cast<int32_t>(1u << 26),
    static_cast<int32_t>(1u << 25), static_cast<int32_t>(1u << 24),
    static_cast<int32_t>(1u << 23), static_cast<int32_t>(1u << 22),
    static_cast<int32_t>(1u << 21), static_cast<int32_t>(1u << 20),
    static_cast<int32_t>(1u << 19), static_cast<int32_t>(1u << 18),
    static_cast<int32_t>(1u << 17), static_cast<int32_t>(1u << 16),
    static_cast<int32_t>(1u << 15), static_cast<int32_t>(1u << 14),
    static_cast<int32_t>(1u << 13), static_cast<int32_t>(1u << 12),
    static_cast<int32_t>(1u << 11), static_cast<int32_t>(1u << 10),
    static_cast<int32_t>(1u << 9),  static_cast<int32_t>(1u << 8)
};
*/
constexpr int32_t shifted_values[24] = 
{
    static_cast<int32_t>(1u << 0),  static_cast<int32_t>(1u << 1),
    static_cast<int32_t>(1u << 2),  static_cast<int32_t>(1u << 3),
    static_cast<int32_t>(1u << 4),  static_cast<int32_t>(1u << 5),
    static_cast<int32_t>(1u << 6),  static_cast<int32_t>(1u << 7),
    static_cast<int32_t>(1u << 8),  static_cast<int32_t>(1u << 9),
    static_cast<int32_t>(1u << 10), static_cast<int32_t>(1u << 11),
    static_cast<int32_t>(1u << 12), static_cast<int32_t>(1u << 13),
    static_cast<int32_t>(1u << 14), static_cast<int32_t>(1u << 15),
    static_cast<int32_t>(1u << 16), static_cast<int32_t>(1u << 17),
    static_cast<int32_t>(1u << 18), static_cast<int32_t>(1u << 19),
    static_cast<int32_t>(1u << 20), static_cast<int32_t>(1u << 21),
    static_cast<int32_t>(1u << 22), static_cast<int32_t>(1u << 23)
};


constexpr uint8_t logical_masks[16][2][2] = {
    {{1, 0}, {1, 0}},  // 0
    {{1, 0}, {0, 0}},  // 1
    {{0, 0}, {1, 0}},  // 2
    {{1, 0}, {1, 1}},  // 3
    {{0, 0}, {0, 0}},  // 4
    {{1, 0}, {0, 1}},  // 5
    {{0, 0}, {1, 1}},  // 6
    {{0, 0}, {0, 1}},  // 7
    {{1, 1}, {1, 0}},  // 8
    {{1, 1}, {0, 0}},  // 9
    {{0, 1}, {1, 0}},  //10
    {{0, 1}, {0, 0}},  //11
    {{1, 1}, {1, 1}},  //12
    {{1, 1}, {0, 1}},  //13
    {{0, 1}, {1, 1}},  //14
    {{0, 1}, {0, 1}}   //15
};

PEncoder::PEncoder () :
m_x(-1), m_y(-1), m_h(-1), m_w(-1),
m_textureID(0), m_pbo(0), m_cuda_pbo_resource(nullptr), m_texture_initialized(false)
{
    /* Loads mask and stores it to reduce memory calls */
    masks = torch::from_blob(
        (void*)logical_masks,
        {16, 2, 2},
        torch::TensorOptions().dtype(torch::kUInt8)
    ).clone().to(DEVICE);

    shifts = torch::from_blob(
        (void*)shifted_values,
        {24},
        torch::TensorOptions().dtype(torch::kInt32)
    ).clone().to(DEVICE);
}

PEncoder::PEncoder (int x, int y, int h, int w) :
m_x(x), m_y(y), m_h(h), m_w(w),
m_textureID(0), m_pbo(0), m_cuda_pbo_resource(nullptr), m_texture_initialized(false)
{
    assert (m_h % 2 == 0);
    assert (m_w % 2 == 0);

    /* Loads mask and stores it to reduce memory calls */
    masks = torch::from_blob(
        (void*)logical_masks,
        {16, 2, 2},
        torch::TensorOptions().dtype(torch::kUInt8)
    ).clone().to(DEVICE);

    shifts = torch::from_blob(
        (void*)shifted_values,
        {24},
        torch::TensorOptions().dtype(torch::kInt32)
    ).clone().to(DEVICE);
}

PEncoder::~PEncoder () {
    #if defined(__linux__)
    if (m_texture_initialized) {
        if (m_cuda_pbo_resource) {
            cudaGraphicsUnregisterResource(m_cuda_pbo_resource);
            m_cuda_pbo_resource = nullptr;
        }
        if (m_pbo) {
            glDeleteBuffers(1, &m_pbo);
            m_pbo = 0;
        }
        if (m_textureID) {
            glDeleteTextures(1, &m_textureID);
            m_textureID = 0;
        }
        m_texture_initialized = false;
    }
    #endif
}

u8Image PEncoder::Encode_u8Image (torch::Tensor &x) {
    torch::Tensor encoding = Encode_u8Tensor(x).contiguous();
    auto* data_ptr = encoding.data_ptr<uint8_t>();
    auto total_size = encoding.numel();

    u8Image result(data_ptr, data_ptr + total_size);
    return result;
}

Image PEncoder::Encode_Image (torch::Tensor &x) {
    auto y = Encode_u8Tensor(x);
    return u8Tensor_Image(y);
}

torch::Tensor PEncoder::Encode_u8Tensor (torch::Tensor &x) {
    if (x.device() != masks.device())
        masks = masks.to(x.device());   /* Assign device correctly */

    /**
     * Check if x is of shape [H, W] or [N, H, W]
     * if [H, W] continue, but if [N, H, W], go to MEncode_u8Tensor
     */
    if (x.dim() == 3) {
        return MEncode_u8Tensor(x);
    }

    auto plane  = torch::zeros(std::vector<int64_t>{m_h/2, m_w/2}, x.options());

    auto x_h = x.size(0);
    auto x_w = x.size(1);
    plane.slice(0, m_x, m_x + x_h).slice(1, m_y, m_y + x_w) = x;

    auto qplane = q(plane, true);  // [H/2, W/2], indices into logical_masks
    auto logical = masks.index_select(0, qplane.view(-1)).to(x.device());         // [H/2 * W/2, 2, 2]
    logical = logical.view({qplane.size(0), qplane.size(1), 2, 2}); // [H/2, W/2, 2, 2]
    logical = logical.permute({0, 2, 1, 3}).contiguous();           // [H/2,2,W/2,2]
    logical = logical.view({qplane.size(0) * 2, qplane.size(1) * 2}); // [H, W]
    return logical.to(torch::kUInt8);
}

torch::Tensor PEncoder::MEncode_u8Tensor (torch::Tensor &x) {
    validate_args();    /* Validate input arguments m_* */
    int64_t N = x.size(0);
    if (N > 24) {
        throw std::runtime_error("PEncoder::MEncode_u8Tensor: x must be [N, H, W], where N <= 24, N given is: " + std::to_string(N) + "\n");
    }


    torch::Tensor image = torch::zeros(std::vector<int64_t>{m_h, m_w}, x.options()).to(torch::kInt32);

    for (int64_t i = 0; i < N; ++i) {
        torch::Tensor xi = x[i];                       // [h, w]
        torch::Tensor yi = Encode_u8Tensor(xi);        // [H, W]
        image = image | (yi.to(torch::kInt32).mul_(1u << (32-i))); // Avoid << on tensors, use mul_ with shifted scalar
    }

    // This encodes [N, H, W] to binary [H, W] for each pixel, it leaves the alpha channel alone (only 24-bits)
    return image;
}

torch::Tensor PEncoder::MEncode_u8Tensor2 (const torch::Tensor &x) {
    validate_args();

    int64_t N = x.size(0);
    if (N > 24) {
        throw std::runtime_error("PEncoder::MEncode_u8Tensor2: x must be [N, H, W], where N <= 24, N given is: " + std::to_string(N) + "\n");
    }


    torch::Tensor image = torch::zeros(std::vector<int64_t>{m_h, m_w}, x.options()).to(torch::kInt32);
    torch::Tensor plane = torch::zeros(std::vector<int64_t>{N, m_h >> 1, m_w >> 1}, x.options()).to(torch::kFloat32);

    plane.slice(1, m_x, m_x + x.size(1))
     .slice(2, m_y, m_y + x.size(2))
     .copy_(x);

    //plane = q(plane, true);
    auto t1 = Utils::GetCurrentTime_us ();
    plane = q[plane];
    Utils::SynchronizeCUDADevices();
    auto t2 = Utils::GetCurrentTime_us ();
    std::cout<< "INFO: [PEncoder::MEncode_u8Tensor2] Quantization Mapping took: " << (t2 - t1) << " us\n";

    torch::Tensor encoded = torch::zeros({N, m_h, m_w}, x.options().dtype(torch::kUInt8));

    auto logical = masks.index_select(0, plane.view({-1})).to(x.device());
    logical = logical.view({N, m_h / 2, m_w / 2, 2, 2});
    logical = logical.permute({0, 1, 3, 2, 4}).contiguous();
    encoded = logical.view({N, m_h, m_w});

    // Use precomputed shifts tensor from class, slice to N, move to device, and reshape
    torch::Tensor shifts_used = shifts.index({torch::indexing::Slice(0, N)}).to(x.device()).view({N, 1, 1});
    encoded = encoded.to(torch::kInt32);
    image = torch::sum(encoded * shifts_used, 0);

    // This encodes [N, H, W] to binary [H, W] for each pixel, it leaves the alpha channel alone (only 24-bits)
    return image.to(torch::kInt32);
}

torch::Tensor PEncoder::MEncode_u8Tensor2 (torch::Tensor &x) {
    validate_args();

    int64_t N = x.size(0);
    if (N > 24) {
        throw std::runtime_error("PEncoder::MEncode_u8Tensor2: x must be [N, H, W], where N <= 24, N given is: " + std::to_string(N) + "\n");
    }

    auto t1_b = Utils::GetCurrentTime_us ();
    torch::Tensor image = torch::zeros(std::vector<int64_t>{m_h, m_w}, x.options()).to(torch::kInt32);
    torch::Tensor plane = torch::zeros(std::vector<int64_t>{N, m_h >> 1, m_w >> 1}, x.options()).to(torch::kFloat16).to(x.device());

    plane.slice(1, m_x, m_x + x.size(1))
     .slice(2, m_y, m_y + x.size(2))
     .copy_(x);

    //plane = q(plane, true);
    plane = q[plane];
    Utils::SynchronizeCUDADevices();
    auto t2_b = Utils::GetCurrentTime_us ();
    std::cout<< "INFO: [PEncoder::MEncode_u8Tensor2] Quantization Mapping took: " << (t2_b - t1_b) << " us\n";


    torch::Tensor encoded = torch::zeros({N, m_h, m_w}, x.options().dtype(torch::kUInt8));

    auto logical = masks.index_select(0, plane.view({-1})).to(x.device());
    logical = logical.view({N, m_h / 2, m_w / 2, 2, 2});
    logical = logical.permute({0, 1, 3, 2, 4}).contiguous();
    encoded = logical.view({N, m_h, m_w});

    // Use precomputed shifts tensor from class, slice to N, move to device, and reshape
    torch::Tensor shifts_used = shifts.index({torch::indexing::Slice(0, N)}).to(x.device()).view({N, 1, 1});
    encoded = encoded.to(torch::kInt32);
    image = torch::sum(encoded * shifts_used, 0);
    Utils::SynchronizeCUDADevices();
    auto t3_b = Utils::GetCurrentTime_us();
    std::cout<< "INFO: [PEncoder::MEncode_u8Tensor2] Spatial-bit Mapping took " << (t3_b - t2_b) << " us\n";

    // This encodes [N, H, W] to binary [H, W] for each pixel, it leaves the alpha channel alone (only 24-bits)
    return image.to(torch::kInt32);

}

torch::Tensor PEncoder::upscale_ (const torch::Tensor &x, int scale_h, int scale_w) {
    int64_t H = x.size(0) * scale_h;
    int64_t W = x.size(1) * scale_w;

    auto x_blocks = x.unfold(0, 2, 2).unfold(1, 2, 2); // [N, H/2, W/2, 2, 2]
    x_blocks = x_blocks.repeat_interleave(scale_h, 0).repeat_interleave(scale_w, 1);
    return x_blocks.permute({0,2,1,3}).reshape({H, W}).contiguous();
}
torch::Tensor PEncoder::MEncode_u8Tensor3 (const torch::Tensor &x) {
    int64_t N       = x.size(0);
    int64_t input_h = x.size(1);
    int64_t input_w = x.size(2);

    // firstly, quantize the input tensor first (which is often much smaller than x)
    torch::Tensor plane = q[x];

    std::cout<<"INFO: [PEncoder::MEncode_u8Tensor3] Generating encoding...\n";
    if (masks.device() != x.device())
        masks = masks.to(x.device());

    auto logical = masks.index_select(0, plane.view({-1}));
    logical = logical.view({N, input_h, input_w, 2, 2});
    logical = logical.permute({0, 1, 3, 2, 4}).contiguous();
    torch::Tensor encoded = logical.view({N, input_h * 2, input_w * 2});

    std::cout<<"INFO: [PEncoder::MEncode_u8Tensor3] Generating bit representation...\n";

    // Use precomputed shifts tensor from class, slice to N, move to device, and reshape
    torch::Tensor shifts_used = shifts.index({torch::indexing::Slice(0, N)}).to(x.device()).view({N, 1, 1});
    encoded = encoded.to(torch::kInt32);
    torch::Tensor image = torch::sum(encoded * shifts_used, 0);

    // image: [iH*2, iW*2] — already in block resolution
    int64_t raw_h = image.size(0);  // iH * 2
    int64_t raw_w = image.size(1);  // iW * 2

    int64_t scale_h = m_h / raw_h;
    int64_t scale_w = m_w / raw_w;

    // Sanity check
    TORCH_CHECK(m_h == raw_h * scale_h && m_w == raw_w * scale_w,
        "m_h and m_w must be multiples of input image size");

    return upscale_(image, scale_h, scale_w).to(torch::kInt32);
}

torch::Tensor PEncoder::MEncode_u8Tensor4 (const torch::Tensor &x) {
    int64_t N       = x.size(0);
    int64_t input_h = x.size(1);
    int64_t input_w = x.size(2);

    /* Apply normalization */
    auto norm_x = (x + M_PI) / (2 * M_PI); // Normalize to [0, 1]
    auto mean_x = norm_x.mean();
    auto binary_x = (norm_x > mean_x/*0.3197875,0.5*/).to(torch::kInt32); // Convert to binary [0, 1]

    torch::Tensor shifts_used = shifts.index({torch::indexing::Slice(0, N)}).to(x.device()).view({N, 1, 1});
    torch::Tensor down_image = torch::sum(binary_x * shifts_used, 0).to(torch::kFloat64);

    /* Upscale x using torchvision */
    auto image = torch::nn::functional::interpolate(
        down_image.unsqueeze(0).unsqueeze(0),    // Add batch dimension [1, 1, H, W]
        torch::nn::functional::InterpolateFuncOptions()
            .size(std::vector<int64_t>{static_cast<int64_t>(m_h), static_cast<int64_t>(m_w)})   // Scale to [H, W]
            .mode(torch::kNearest)
    ).squeeze().to(torch::kInt32);   // [N, H, W]  

    return image;
}

torch::Tensor PEncoder::MEncode_u8Tensor5 (const torch::Tensor &x) {
    int64_t N       = x.size(0);
    int64_t input_h = x.size(1);
    int64_t input_w = x.size(2);

    // firstly, quantize the input tensor first (which is often much smaller than x)
    torch::Tensor plane = q[x];

    std::cout<<"INFO: [PEncoder::MEncode_u8Tensor3] Generating encoding...\n";
    if (masks.device() != x.device())
        masks = masks.to(x.device());

    auto logical = masks.index_select(0, plane.view({-1}));
    logical = logical.view({N, input_h, input_w, 2, 2});
    logical = logical.permute({0, 1, 3, 2, 4}).contiguous();
    torch::Tensor encoded = logical.view({N, input_h * 2, input_w * 2});

    std::cout<<"INFO: [PEncoder::MEncode_u8Tensor3] Generating bit representation...\n";

    // Use precomputed shifts tensor from class, slice to N, move to device, and reshape
    torch::Tensor shifts_used = shifts.index({torch::indexing::Slice(0, N)}).to(x.device()).view({N, 1, 1});
    encoded = encoded.to(torch::kInt32);
    torch::Tensor image = torch::sum(encoded * shifts_used, 0);

    return image;
}



Image PEncoder::u8Tensor_Image (torch::Tensor &x) {
    torch::Tensor encoding = MEncode_u8Tensor(x).contiguous();
    return u8MTensor_Image (encoding);
}

Image PEncoder::u8MTensor_Image (torch::Tensor &x) {
    int32_t *data_ptr = nullptr;
    torch::Tensor tmp;
    if (x.device() == torch::kCPU)
        tmp = x;
    else
        tmp = x.cpu();
    data_ptr = tmp.data_ptr<int32_t>();
    auto total_size = x.numel();
    if (!data_ptr) throw std::runtime_error("ERROR: [pencoder::u8MTensor_Image] data pointer is NULL\n");

    uint8_t* data = new uint8_t[total_size*sizeof(int32_t)];

    std::memcpy(data, data_ptr, total_size*sizeof(int32_t));

    Image ret {
        .data    = data,
        .width   = m_w,
        .height  = m_h,
        .mipmaps = 1,
        .format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    return ret;
}

#if defined(__linux__)
Texture PEncoder::u8Tensor_Texture (torch::Tensor &encoding) {
    TORCH_CHECK(encoding.device().is_cuda(), "encoding must be on CUDA");
    TORCH_CHECK(encoding.is_contiguous(), "encoding must be contiguous");

    int64_t H = encoding.size(0);
    int64_t W = encoding.size(1);

    if (!m_texture_initialized) {
        init_pbo();
    }

    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo);

    cudaGraphicsMapResources(1, &m_cuda_pbo_resource, 0);
    uint8_t* cuda_ptr = nullptr;
    size_t size;
    cudaGraphicsResourceGetMappedPointer((void**)&cuda_ptr, &size, m_cuda_pbo_resource);
    cudaMemcpy(cuda_ptr, encoding.data_ptr(), encoding.numel() * sizeof(int32_t), cudaMemcpyDeviceToDevice);
    cudaGraphicsUnmapResources(1, &m_cuda_pbo_resource, 0);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    Texture texture = { 0 };
    texture.id = m_textureID;
    texture.width = W;
    texture.height = H;
    texture.mipmaps = 1;
    texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    return texture;
}
#else
Texture PEncoder::u8Tensor_Texture (torch::Tensor &encoding) {
    Image image = u8Tensor_Image (encoding);
    Texture texture = LoadTextureFromImage (image);
    UnloadImage (image);
    return texture;
}
#endif

torch::Tensor PEncoder::ImageTensorMap (torch::Tensor &x1, torch::Tensor &i0) {
    auto s1 = x1.sizes();
    auto s2 = i0.sizes();

    /* Use BImageTensorMap instead */
    if (s2.size() < 3 || s2[0] == 1) {
        return BImageTensorMap(x1, i0);
    }

    if (s1 != s2) {
        throw std::runtime_error("PEncoder::ImageTensorMap: Sizes must match.\n");
    }

    int64_t N = x1.size(0);
    auto result = x1 * i0;
    return result;
}

torch::Tensor PEncoder::BImageTensorMap (torch::Tensor &x1, torch::Tensor &i0) {
    auto i0_broadcasted = (i0.size(0) == 1) ? i0 : i0.unsqueeze(0); // shape [1, H, W]
    return x1 * i0_broadcasted; // broadcasted multiplication
}

void PEncoder::validate_args () {
    if (m_x < 0 || m_y < 0 || m_h < 0 || m_w < 0)
        throw std::runtime_error("PEncoder::validate_args: You may not have initialized arguments m_x, m_y, m_h, and m_w");
}

#if defined(__linux__)
void PEncoder::init_pbo () {
    if (m_texture_initialized) return;  /* already init'd */

    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_w, m_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenBuffers(1, &m_pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, m_w * m_h * sizeof(uint32_t), nullptr, GL_STREAM_DRAW);

    cudaGraphicsGLRegisterBuffer(&m_cuda_pbo_resource, m_pbo, cudaGraphicsMapFlagsWriteDiscard);

    m_texture_initialized = true;
}
#else
void PEncoder::init_pbo () {
    m_texture_initialized = true;
}
#endif