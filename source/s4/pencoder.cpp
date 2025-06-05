#include "pencoder.hpp"
#include <iostream>
#include <cstdint>
#include <stdexcept>

#include "../device.hpp"    /* Includes device macro */

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
m_x(-1), m_y(-1), m_h(-1), m_w(-1)
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
m_x(x), m_y(y), m_h(h), m_w(w)
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
        throw std::runtime_error("PEncoder::MEncode_u8Tensor: x must be [N, H, W], where N <= 24");
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

torch::Tensor PEncoder::MEncode_u8Tensor2 (torch::Tensor &x) {
    validate_args();

    int64_t N = x.size(0);
    if (N > 24) {
        throw std::runtime_error("PEncoder::MEncode_u8Tensor: x must be [N, H, W], where N <= 24");
    }

    torch::Tensor image = torch::zeros(std::vector<int64_t>{m_h, m_w}, x.options()).to(torch::kInt32);
    torch::Tensor plane = torch::zeros(std::vector<int64_t>{N, m_h >> 1, m_w >> 1}, x.options()).to(torch::kFloat32);

    plane.slice(1, m_x, m_x + x.size(1))
     .slice(2, m_y, m_y + x.size(2))
     .copy_(x);

    //plane = q(plane, true);
    plane = q[plane];

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

Image PEncoder::u8Tensor_Image (torch::Tensor &x) {
    torch::Tensor encoding = MEncode_u8Tensor(x).contiguous();
    return u8MTensor_Image (encoding);
}

Image PEncoder::u8MTensor_Image (torch::Tensor &x) {
    int32_t *data_ptr;
    if (x.device() == torch::kCPU)
        data_ptr  = x.data_ptr<int32_t>();
    else
        data_ptr  = x.cpu().data_ptr<int32_t>();
    auto total_size = x.numel();

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