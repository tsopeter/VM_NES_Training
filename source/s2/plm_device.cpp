#include "plm_device.hpp"
#include "../utils/utils.hpp"

PLM_Device::PLM_Device(PLM_Device_Enum device, int num_levels) {
    set_device(device, num_levels);
}

PLM_Device::~PLM_Device() {
    /* Nothing to clean up for now */
}

torch::Tensor PLM_Device::mapper(torch::Tensor &x) {
    switch (m_device) {
        case PLM_Device_Enum::VISIBLE:
            
            switch (m_levels) {
                case 2:
                    x = torch::where(x == 0, torch::zeros_like(x), x);
                    x = torch::where(x == 1, torch::full_like(x, 11), x);
                    break;
                case 4:
                    x = torch::where(x == 0, torch::zeros_like(x), x);
                    x = torch::where(x == 1, torch::full_like(x, 8), x);
                    x = torch::where(x == 2, torch::full_like(x, 13), x);
                    x = torch::where(x == 3, torch::full_like(x, 15), x);
                    break;
                case 8:
                    x = torch::where(x == 0, torch::zeros_like(x), x);
                    x = torch::where(x == 1, torch::full_like(x, 6), x);
                    x = torch::where(x == 2, torch::full_like(x, 8), x);
                    x = torch::where(x == 3, torch::full_like(x, 10), x);
                    x = torch::where(x == 4, torch::full_like(x, 12), x);
                    x = torch::where(x == 5, torch::full_like(x, 13), x);
                    x = torch::where(x == 6, torch::full_like(x, 14), x);
                    x = torch::where(x == 7, torch::full_like(x, 15), x);
                    break;
                case 16:
                    /* Do nothing */
                    break;
                default:
                    throw std::runtime_error("Unsupported number of levels: " + std::to_string(m_levels) + " for device: " + std::to_string(m_device) + "\n");
            }

        case PLM_Device_Enum::NIR:
            
            switch (m_levels) {
                case 32:
                    /* For 32 levels, we can directly use the input as the output, since it already maps to 0-31 which corresponds to the logical masks */
                    break;
                default:
                    throw std::runtime_error("Unsupported number of levels: " + std::to_string(m_levels) + " for device: " + std::to_string(m_device) + "\n");
            }

             break;

        default:
            throw std::runtime_error("Unsupported PLM device type: " + std::to_string(m_device) + "\n");
    }
    return x;
}

void PLM_Device::set_device(PLM_Device_Enum device, int num_levels) {
    m_device = device;
    m_levels = num_levels;

    switch (device) {
        case PLM_Device_Enum::VISIBLE:
            m_max_num_levels = 16;

            // supported levels
            m_supported_levels = {
                16
            };

            break;
        case PLM_Device_Enum::NIR:
            m_max_num_levels = 32;

            // supported levels (so far)
            m_supported_levels = {
                32
            };

            break;
        default:
            throw std::runtime_error("Unsupported PLM device type: " + std::to_string(device) + "\n");
    }

    // If num_levels is not in supported levels, throw error
    if (std::find(m_supported_levels.begin(), m_supported_levels.end(), num_levels) == m_supported_levels.end()) {
        throw std::runtime_error("Unsupported number of levels: " + std::to_string(num_levels) + " for device: " + std::to_string(device) + "\n");
    }

    // Depending on the device, we have different mappings,
    switch (m_device) {
        case PLM_Device_Enum::VISIBLE:
            m_table = torch::tensor(
                {
                    0.0000, 0.0100, 0.0205, 0.0422,
                    0.0560, 0.0727, 0.1131, 0.1734,
                    0.3426, 0.3707, 0.4228, 0.4916,
                    0.5994, 0.6671, 0.7970, 0.9375
                },
                torch::TensorOptions().dtype(torch::kFloat32)
            );
            break;
        case PLM_Device_Enum::NIR:
            
            // The quantization table is
            // different for even and odd mirror columns
            // We use the first 32 values for odd columns and the next 32 values for even columns
            m_table = torch::tensor(
                {
                    // odd
                    0.0000, 0.0127, 0.0293, 0.0662,
                    0.0522, 0.0675, 0.0854, 0.1261,
                    0.1427, 0.1834, 0.2166, 0.2803,
                    0.2561, 0.2968, 0.3299, 0.3962,
                    0.3771, 0.4102, 0.4510, 0.5108,
                    0.4930, 0.5261, 0.5682, 0.6306,
                    0.6127, 0.6675, 0.7338, 0.8229,
                    0.7847, 0.8369, 0.9045, 1.0000,

                    // even
                    0.0064, 0.0153, 0.0280, 0.0599,
                    0.0611, 0.0726, 0.0866, 0.1223,
                    0.1414, 0.1771, 0.2076, 0.2675,
                    0.2573, 0.2930, 0.3236, 0.3847,
                    0.3911, 0.4178, 0.4586, 0.5121,
                    0.5134, 0.5414, 0.5809, 0.6357,
                    0.6255, 0.6713, 0.7350, 0.8127,
                    0.8038, 0.8446, 0.9057, 0.9822
                },
                torch::TensorOptions().dtype(torch::kFloat32)
            );
            break;
        default:
            throw std::runtime_error("Unsupported PLM device type: " + std::to_string(m_device) + "\n");
    }
}

torch::Tensor PLM_Device::operator[](const torch::Tensor &x) {
    switch (m_device) {
        case PLM_Device_Enum::VISIBLE:
            return operator_implt_visible(x);
        case PLM_Device_Enum::NIR:
            return operator_implt_nir(x);
        default:
            throw std::runtime_error("Unsupported PLM device type: " + std::to_string(m_device) + "\n");
    }
}

torch::Tensor PLM_Device::operator_implt_visible(const torch::Tensor &x) {
    if (x.device() != m_table.device())
        m_table = m_table.to(x.device()).to(x.dtype());

    if (x.device() != m_table_r.device())
        m_table_r = m_table.to(x.device()).to(x.dtype()).view({1, -1});

    auto normalized_x = (x + M_PI) / (2 * M_PI);  // [0, 1]
    auto flat_x = normalized_x.reshape({-1});        // [N]

    // [N, L] = [N, 1] - [1, L] with broadcasting
    auto t1 = Utils::GetCurrentTime_us();
    auto diffs = torch::remainder(flat_x.unsqueeze(1) - m_table_r + 0.5, 1.0) - 0.5;
    Utils::SynchronizeCUDADevices();

    auto t2 = Utils::GetCurrentTime_us();

    // [N]
    auto indices = torch::argmin(diffs.abs(), 1);
    Utils::SynchronizeCUDADevices();
    auto t3 = Utils::GetCurrentTime_us();
    std::cout<<"INFO: [Quantize::operator[]] Remainder took: " << (t2 - t1) << " us\n";
    std::cout<<"INFO: [Quantize::operator[]] Argmin    took: " << (t3 - t2) << " us\n";

    return indices.view_as(x).contiguous();
}

torch::Tensor PLM_Device::operator_implt_nir(const torch::Tensor &x) {
    throw std::runtime_error("NIR device operator not implemented yet\n");
}