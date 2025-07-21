#include <iostream>
#include <cstring>
#include "../source/s3/Serial.hpp"

int main (int argc, const char *argv[]) {
    if (argc != 3) {
        std::cerr<<"./"<<argv[0]<<" <port_name> <baud_rate>\n";
        return -1;
    }

    Serial serial {argv[1], std::atoi(argv[2])};
    try {
        serial.Open();
    } catch (std::exception &e) {
        std::cerr<<e.what()<<'\n';
        return -1;
    }

    std::string x;
    while (true) {
        std::cin>>x;
        if (x == "exit")
            break;
        serial.Send(x);
    }

    serial.Close();
    return 0;
}