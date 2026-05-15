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
    else {
        // Invalid arguments
        std::cerr << "Usage for training:  " << argv[0] << " <config>\n";
        return -1;
    }

    return 0;
}       
