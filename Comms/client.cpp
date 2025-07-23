#include "../source/s3/IP.hpp"
#include <iostream>
#include <cstring>

int main () {
    const char str[] = "Hello, World!";
    std::string ip = "192.168.193.20";
    s3_IP_Client client(ip, 9000);
    if (!client.connect()) {
        std::cerr << "Unable to connect to: " << ip << '\n';
    }
    client.Transmit((void*)str, strlen(str));
    client.disconnect();
}