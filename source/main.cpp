#include <iostream>
#include "runner.hpp"
#include "s5/helpers.hpp"
#include "s2/dataloader.hpp"

int main (int argc, char** argv) {
    std::string config_file = "config.txt";
    Runner r;


    // If training mode, the argument is structured as ./app <config>
    if (argc == 2) {
        config_file = argv[1];
        r.Run (config_file);
    }
    // If inference mode, the argument is structured as ./app -i <config> <dataset> <dataset size>
    else if (argc == 5 && std::string(argv[1]) == "-i") {
        config_file = argv[2];
        r.m_inference = true;

        // Parse dataset type
        // Supported dataset types: train, valid, test
        s2_DataTypes dtype;
        std::string dataset_str = argv[3];
        if (dataset_str == "train") {
            dtype = s2_DataTypes::TRAIN;
        }
        else if (dataset_str == "valid") {
            dtype = s2_DataTypes::VALID;
        }
        else if (dataset_str == "test") {
            dtype = s2_DataTypes::TEST;
        }
        else {
            throw std::runtime_error("Runner::Inference: Unsupported dataset type specified.");
        }

        int n_data_points = std::stoi(argv[4]);
        r.Inference (config_file, dtype, n_data_points);
    }
    else if (argc == 6 && std::string(argv[1]) == "-s") {
        config_file = argv[2];
        r.m_inference = true;

        // Parse dataset type
        // Supported dataset types: train, valid, test
        s2_DataTypes dtype;
        std::string dataset_str = argv[3];
        if (dataset_str == "train") {
            dtype = s2_DataTypes::TRAIN;
        }
        else if (dataset_str == "valid") {
            dtype = s2_DataTypes::VALID;
        }
        else if (dataset_str == "test") {
            dtype = s2_DataTypes::TEST;
        }
        else {
            throw std::runtime_error("Runner::StaticInference: Unsupported dataset type specified.");
        }

        int n_data_points = std::stoi(argv[4]);
        std::string mask_file = argv[5];
        r.StaticInference (config_file, dtype, n_data_points, mask_file);
    }
    else {
        // Invalid arguments
        std::cerr << "Usage for training:  " << argv[0] << " <config>\n";
        std::cerr << "\tExample usage: " << argv[0] << " config.txt\n";
        std::cerr << "Usage for inference: " << argv[0] << " -i <checkpoint_config> <dataset> <dataset size>\n";
        std::cerr << "\tExample usage: " << argv[0] << " -i checkpoint_config.txt test 1000\n";
        std::cerr << "Usage for inference with mask from image file: " << argv[0] << " -s <checkpoint_config> <dataset> <dataset size> <mask file>\n";
        std::cerr << "\tExample usage: " << argv[0] << " -s checkpoint_config.txt test 1000 mask.png\n";
        return -1;
    }

    return 0;
}       
