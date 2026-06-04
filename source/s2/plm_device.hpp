#ifndef s2_plm_device_hpp__
#define s2_plm_device_hpp__

#include <vector>
#include <torch/torch.h>

enum PLM_Device_Enum : int {
    VISIBLE = 0,
    NIR     = 1,
    NIR2    = 2
};

struct PLM_Device {
    PLM_Device(PLM_Device_Enum device=PLM_Device_Enum::VISIBLE, int num_levels=16);
    ~PLM_Device();

    void set_device(PLM_Device_Enum device, int num_levels);

    int m_max_num_levels;
    int m_levels;
    PLM_Device_Enum m_device;
    torch::Tensor m_table; // Quantization table for the device
    torch::Tensor m_table_r;
    std::vector<int> m_supported_levels;

    torch::Tensor mapper(torch::Tensor &x);

    torch::Tensor operator[](const torch::Tensor &x);
    torch::Tensor operator_implt_visible(const torch::Tensor &x);
    torch::Tensor operator_implt_nir(const torch::Tensor &x);
    torch::Tensor operator_implt_nir2(const torch::Tensor &x);

};



#endif 