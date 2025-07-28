#include <thread>
#include <cstring>  // for strlen
#include <netinet/in.h>
#include <unistd.h>
#include "e21.hpp"
#include "../s3/IP.hpp"

int e21 () {
    std::thread receiver([]() {
        s3_IP_Host server(9000);
        if (!server.listen_and_accept()) {
            std::cerr << "Server failed to start.\n";
            return;
        }

        char buffer[128] = {0};
        server.Receive(buffer, sizeof(buffer));
        std::cout << "Received: " << buffer << std::endl;

        server.disconnect();
    });

    std::thread sender([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait for receiver to set up
        s3_IP_Client client("127.0.0.1", 9000);
        if (client.connect()) {
            const char* msg = "Hello";
            client.Transmit((void*)msg, strlen(msg));
            client.disconnect();
        }
    });

    receiver.join();
    sender.join();
    return 0;
}