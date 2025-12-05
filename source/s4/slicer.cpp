#include "slicer.hpp"

// Defaults
s4_Slicer::s4_Slicer () {}
s4_Slicer::~s4_Slicer () {}

s4_Slicer::s4_Slicer(s4_Slicer_Region_Vector regions, int h, int w) {
    std::vector<torch::Tensor> tensors;
    for (const auto& r : regions)
        tensors.push_back(r->get_region(h, w));
    m_regions = torch::stack(tensors);
}

// Construct from vector of torch::Tensor
s4_Slicer::s4_Slicer(std::vector<torch::Tensor> regions) {
    m_regions = torch::stack(regions);
}

// Construct from vector of u8Image
s4_Slicer::s4_Slicer(std::vector<u8Image> regions) {
    std::vector<torch::Tensor> tensors;
    for (const auto& img : regions) {
        torch::Tensor t = torch::from_blob(
            const_cast<uint8_t*>(img.data()),
            {static_cast<long>(img.size())},
            torch::TensorOptions().dtype(torch::kUInt8)
        ).clone();
        tensors.push_back(t);
    }
    m_regions = torch::stack(tensors);
}

s4_Slicer::s4_Slicer (torch::Tensor regions)
: m_regions(regions) {}

torch::Tensor s4_Slicer::detect(torch::Tensor t) {
    TORCH_CHECK(t.dim() == 2 || t.dim() == 3, "Input must be 2D [H, W] or 3D [N, H, W]");
    TORCH_CHECK(m_regions.dim() == 3, "Region tensor must have shape [N, H, W]");

    if (t.dim() == 2) {
        t = t.unsqueeze(0);  // convert to [1, H, W]
    }


    //

    TORCH_CHECK(t.sizes().slice(1) == m_regions.sizes().slice(1),
                "Input and region spatial dimensions must match");

    auto t_expanded = t.unsqueeze(1); // [B, 1, H, W]
    auto r_expanded = m_regions.unsqueeze(0).to(t.device()); // [1, N, H, W]
    auto scores = (t_expanded * r_expanded).sum({2, 3}); // [B, N]

    return scores;
}

torch::Tensor s4_Slicer::detect(u8Image i) {
    torch::Tensor t = torch::from_blob(
        const_cast<uint8_t*>(i.data()),
        {static_cast<long>(i.size())},
        torch::TensorOptions().dtype(torch::kUInt8)
    ).clone().unsqueeze(0);
    return detect(t);
}

torch::Tensor s4_Slicer::detect (std::vector<u8Image> is) {
    std::vector<torch::Tensor> tensors;
    for (const auto& img : is) {
        torch::Tensor t = torch::from_blob(
            const_cast<uint8_t*>(img.data()),
            {static_cast<long>(img.size())},
            torch::TensorOptions().dtype(torch::kUInt8)
        ).clone();
        tensors.push_back(t);
    }
    return detect(tensors);
}

torch::Tensor s4_Slicer::detect (std::vector<torch::Tensor> ts) {
    // ts would have shape [N,...], where N = ts.size()
    auto T = torch::stack(ts);
    return detect(T);
}

Image s4_Slicer::visualize() {
    TORCH_CHECK(m_regions.dim() == 3, "Regions must have shape [N, H, W]");

    auto sum = m_regions.sum(0).to(torch::kU8); // [H, W]
    sum = torch::where(sum > 0, 255, 0).to(torch::kU8);
    auto H = sum.size(0);
    auto W = sum.size(1);

    Image img;
    img.width = W;
    img.height = H;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
    img.data = malloc(W * H);
    std::memcpy(img.data, sum.data_ptr(), W * H);

    return img;
}

void s4_Slicer::device (torch::Device device) {
    m_regions = m_regions.to(device);
}

////////////////////////////////////////////////////////
// Implementation of Slicer Regions                   //
////////////////////////////////////////////////////////

s4_Slicer_Circle::s4_Slicer_Circle (int x, int y, int r)
: m_x(x), m_y(y), m_r(r) {}
s4_Slicer_Circle::~s4_Slicer_Circle () {}

torch::Tensor s4_Slicer_Circle::get_region(int h, int w) {
    auto yy = torch::arange(h, torch::kInt32).view({-1, 1}).expand({h, w});
    auto xx = torch::arange(w, torch::kInt32).view({1, -1}).expand({h, w});
    auto dist2 = (xx - m_x).pow(2) + (yy - m_y).pow(2);
    return (dist2 <= m_r * m_r).to(torch::kUInt8);
}

s4_Slicer_Square::s4_Slicer_Square (int x, int y, int r)
: m_x(x), m_y(y), m_r(r) {}
s4_Slicer_Square::~s4_Slicer_Square () {}

torch::Tensor s4_Slicer_Square::get_region(int h, int w) {
    auto yy = torch::arange(h, torch::kInt32).view({-1, 1}).expand({h, w});
    auto xx = torch::arange(w, torch::kInt32).view({1, -1}).expand({h, w});
    auto x_mask = (xx >= (m_x - m_r)) & (xx <= (m_x + m_r));
    auto y_mask = (yy >= (m_y - m_r)) & (yy <= (m_y + m_r));
    return (x_mask & y_mask).to(torch::kUInt8);
}