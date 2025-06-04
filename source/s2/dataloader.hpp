#ifndef s2_dataloader_hpp__
#define s2_dataloader_hpp__
#include <torch/torch.h>
#include <filesystem>
#include <utility>

enum s2_DataTypes : unsigned {
    TRAIN = 0,
    TEST  = 1,
    VALID = 2
};

class s2_Data {
public:
    s2_Data();
    s2_Data(std::filesystem::path &, std::filesystem::path &, int);

    std::pair<torch::Tensor, torch::Tensor> operator[](int);
    std::pair<torch::Tensor, torch::Tensor> operator()(int, int);
    int len() const;

    void set_image_path  (std::filesystem::path);
    void set_label_path  (std::filesystem::path);
    void set_data_length (int);

    void device(torch::Device);
private:
    std::filesystem::path image_path;
    std::filesystem::path label_path;

    void load_data();

    bool loaded    = false;
    torch::Tensor idata, ldata;
    torch::Device m_device = torch::kCPU;
    int sz;
};

class s2_Dataloader {
public:
    /**
     * @brief s2_Dataloader is a class that handles
     *        loading in training/testing/validation data.
     * 
     *        By calling `load` it returns a class
     *        called s2_Data which handles actual data loading.
     * 
     */
    s2_Dataloader(const std::filesystem::path &path);

    s2_Data load (s2_DataTypes, int);
private:
    /* Main Directory where the Datasets are stored */
    std::filesystem::path m_path;
};

#endif
