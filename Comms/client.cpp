#include "../source/s3/IP.hpp"
#include <iostream>
#include <cstring>

int main () {
    const char str[] = "Hello, World!";
    s3_IP_Client client("192.168.193.20", 9000);
    client.Transmit((void*)str, strlen(str));
    client.disconnect();
}