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
    if (argc == 5 && std::string(argv[1]) == "-i") {
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

    return 0;
}       
