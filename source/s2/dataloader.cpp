#include "dataloader.hpp"
#include "np2lt.hpp"

s2_Dataloader::s2_Dataloader(const std::filesystem::path &path) 
: m_path(path)
{}

s2_Data s2_Dataloader::load (s2_DataTypes type, int sz) {
    std::filesystem::path 
    image_path =
        m_path / "Images",
    label_path =
        m_path / "Labels";

    std::string filenames;   
    switch (type) {
        case s2_DataTypes::TRAIN:
            filenames = "Training_Dataset.npy";
            break;
        case s2_DataTypes::TEST:
            filenames = "Test_Dataset.npy";
            break;
        case s2_DataTypes::VALID:
            filenames = "Validation_Dataset.npy";
            break;
        case s2_DataTypes::BALANCED_1:
            filenames = "Balanced_1.npy";
            break;
        case s2_DataTypes::BALANCED_2:
            filenames = "Balanced_2.npy";
            break;
        default:
            filenames = "Validation_Dataset.npy";
    }

    image_path /= filenames;
    label_path /= filenames;

    return s2_Data(image_path, label_path, sz);
}

s2_Data::s2_Data ()
: image_path("."), label_path("."), sz(-1)
{}

s2_Data::s2_Data (std::filesystem::path &p1, std::filesystem::path &p2, int i0)
: image_path(p1), label_path(p2), sz(i0)
{}

std::pair<torch::Tensor, torch::Tensor> s2_Data::operator[](int i) {
    if (!loaded) load_data();
    if (i < 0 || i >= sz) {
        throw std::out_of_range("Invalid index.");
    }
    return {idata[i], ldata[i]};
}

std::pair<torch::Tensor, torch::Tensor> s2_Data::operator()(int i, int n) {
    if (!loaded) load_data();

    int end = std::min(i + n, sz);
    if (i < 0 || i >= sz || end <= i) {
        throw std::out_of_range("Invalid slice range.");
    }

    auto image_slice = idata.index({torch::indexing::Slice(i, end)});
    auto label_slice = ldata.index({torch::indexing::Slice(i, end)});
    return {image_slice, label_slice};
}

void s2_Data::load_data () {
    // load the image data
    auto images = np2lt::f32(image_path).to(m_device);
    // Slice to only the first sz elements
    idata = (images.index({torch::indexing::Slice(0, sz)}) * 255.f).to(torch::kUInt8);

    // load the label data
    auto labels = np2lt::i64(label_path).to(m_device);
    // Slice to only the first sz elements after argmax
    auto argmaxed = torch::argmax(labels, 2);
    ldata = argmaxed.index({torch::indexing::Slice(0, sz)}).view({sz, 1});
    loaded = true;
}

int s2_Data::len() const {
    return sz;
}

void s2_Data::set_image_path (std::filesystem::path p) {
    image_path = p;
    loaded = false;
}

void s2_Data::set_label_path (std::filesystem::path p) {
    label_path = p;
    loaded = false;
}

void s2_Data::set_data_length (int l) {
    sz = l;
    loaded = false;
}

void s2_Data::device(torch::Device d) {
    m_device = d;
    loaded = false;
}