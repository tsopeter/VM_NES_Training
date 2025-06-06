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
    /**
     * @brief s2_Data is a structure that handles loading and partitioning of data files
     * 
     */
    s2_Data();

    /**
     * @brief s2_Data is a structure that handles loading and partitioning of data files
     * 
     * @param image_path Path of image data as a .npy file
     * @param label_path Path of label data as a .npy file
     * @param int        Number of data points to load
     * 
     */
    s2_Data(std::filesystem::path &, std::filesystem::path &, int);

    /**
     * @brief Returns a single instance of data as a tuple [image, label]
     * 
     */
    std::pair<torch::Tensor, torch::Tensor> operator[](int);

    /**
     * @brief Returns multiple instances of data as a tuple[images, labels]
     * 
     */
    std::pair<torch::Tensor, torch::Tensor> operator()(int, int);

    /**
     * @brief Number of datapoints
     * 
     */
    int len() const;

    /**
     * @brief Sets image path; requires reloading if called
     * 
     */
    void set_image_path  (std::filesystem::path);

    /**
     * @brief Sets label path; requires reloading if called
     * 
     */
    void set_label_path  (std::filesystem::path);

    /**
     * @brief Sets number of images to load; requires reloading if called;
     * 
     */
    void set_data_length (int);

    /**
     * @brief Sets the device the images and labels should be on
     * 
     */
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

    /**
     * @brief Loads data from the dataset (typ. "./Dataset")
     * 
     * @param DataTypes Which data to load from TRAIN/TEST/VAL
     * @param int       Number of data points to load
     */
    s2_Data load (s2_DataTypes, int);
private:
    /* Main Directory where the Datasets are stored */
    std::filesystem::path m_path;
};

#endif
