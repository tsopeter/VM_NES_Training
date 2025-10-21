#include <iostream>
#include "runner.hpp"
#include "s5/helpers.hpp"

int main (int argc, char** argv) {
    std::string config_file = "config.txt";

    if (argc == 2) {
        config_file = argv[1];
    }
    Runner r;
    r.Run (config_file);
    return 0;
}       
