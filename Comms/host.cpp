#include "../source/s3/IP.hpp"
#include <iostream>
#include <cstring>

int main () {
    s3_IP_Host server(9000);
    if (!server.listen_and_accept()) {
        std::cerr << "Failed to start server.\n";
        return 1;
    }

    char buffer[1024] = {0};
    int received = server.Receive(buffer);
    if (received > 0) {
        std::cout << "Received: " << std::string(buffer, received) << std::endl;
    }

    server.disconnect();
    return 0;
}