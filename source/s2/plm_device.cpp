#include "plm_device.hpp"
#include "../utils/utils.hpp"

PLM_Device::PLM_Device(PLM_Device_Enum device, int num_levels) {
    set_device(device, num_levels);
}

PLM_Device::~PLM_Device() {
    /* Nothing to clean up for now */
}

torch::Tensor PLM_Device::mapper(torch::Tensor &x) {
    std::cout << "INFO: [PLM_Device::mapper] Mapping input tensor with device: " << static_cast<int>(m_device) << " and levels: " << m_levels << "\n";
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

            break;

        case PLM_Device_Enum::NIR:
        case PLM_Device_Enum::NIR2:
            
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
    std::cout << "INFO: [PLM_Device::set_device] Setting PLM device to " << static_cast<int>(device) << " with num_levels: " << num_levels << "\n";
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
        case PLM_Device_Enum::NIR2:
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
    bool valid_level = false;
    for (auto &lvl : m_supported_levels) {
        if (num_levels == lvl) {
            valid_level = true;
        }
    }
    if (!valid_level) {
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
            m_table_r = m_table.view({1, -1});
            break;
        case PLM_Device_Enum::NIR2:
            // Technically, we don't need
            // odd and even columns
            // since we are using reinforcement learning
            m_table = torch::tensor(
                {
                    0.0000, 0.0127, 0.0293, 0.0662,
                    0.0522, 0.0675, 0.0854, 0.1261,
                    0.1427, 0.1834, 0.2166, 0.2803,
                    0.2561, 0.2968, 0.3299, 0.3962,
                    0.3771, 0.4102, 0.4510, 0.5108,
                    0.4930, 0.5261, 0.5682, 0.6306,
                    0.6127, 0.6675, 0.7338, 0.8229,
                    0.7847, 0.8369, 0.9045, 1.0000
                },
                torch::TensorOptions().dtype(torch::kFloat32)
            );
            m_table_r = m_table.view({1, -1});
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
        case PLM_Device_Enum::NIR2: // Can use visible-style mapping since we treat as single table (no odd/even distinction)
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
    // Input shape: [N, H, W]
    if (x.device() != m_table.device())
        m_table = m_table.to(x.device()).to(x.dtype());

    // Split table into odd and even parts
    auto table_odd = m_table.slice(0, 0, 32);   // First 32 values for odd rows
    auto table_even = m_table.slice(0, 32, 64); // Next 32 values for even rows
    
    auto normalized_x = (x + M_PI) / (2 * M_PI);  // [N, H, W] -> [0, 1]
    
    auto sizes = x.sizes();
    int N = sizes[0];
    int H = sizes[1];
    int W = sizes[2];
    
    // Flatten for processing: [N, H, W] -> [N*H*W]
    auto flat_x = normalized_x.reshape({-1});
    
    // Create row indices: [N*H*W] where each element gets its row number (0 to H-1)
    auto row_indices = torch::arange(H, torch::TensorOptions().device(x.device()))
                              .unsqueeze(1)              // [H, 1]
                              .expand({H, W})            // [H, W]
                              .unsqueeze(0)              // [1, H, W]
                              .expand({N, H, W})         // [N, H, W]
                              .reshape({-1});            // [N*H*W]
    
    // Determine if row is odd (row index % 2 == 1)
    auto is_odd_row = (row_indices % 2 == 1);
    
    // Prepare tables for broadcasting
    auto table_odd_r = table_odd.unsqueeze(0);   // [1, 32]
    auto table_even_r = table_even.unsqueeze(0); // [1, 32]
    
    auto t1 = Utils::GetCurrentTime_us();
    
    // Compute differences for odd rows: [N*H*W, 32]
    auto diffs_odd = torch::remainder(flat_x.unsqueeze(1) - table_odd_r + 0.5, 1.0) - 0.5;
    auto indices_odd = torch::argmin(diffs_odd.abs(), 1);  // [N*H*W]
    
    // Compute differences for even rows: [N*H*W, 32]
    auto diffs_even = torch::remainder(flat_x.unsqueeze(1) - table_even_r + 0.5, 1.0) - 0.5;
    auto indices_even = torch::argmin(diffs_even.abs(), 1);  // [N*H*W]
    
    Utils::SynchronizeCUDADevices();
    auto t2 = Utils::GetCurrentTime_us();
    
    // Select appropriate indices based on row parity: [N*H*W]
    auto indices = torch::where(is_odd_row, indices_odd, indices_even);
    
    Utils::SynchronizeCUDADevices();
    auto t3 = Utils::GetCurrentTime_us();
    
    std::cout << "INFO: [PLM_Device::operator_implt_nir] Quantization took: " << (t2 - t1) << " us\n";
    std::cout << "INFO: [PLM_Device::operator_implt_nir] Selection took: " << (t3 - t2) << " us\n";
    
    // Reshape back to [N, H, W]
    return indices.view({N, H, W}).contiguous();
}