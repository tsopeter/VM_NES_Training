#include <iostream>
#include "examples/e1.hpp"
#include "examples/e2.hpp"
#include "examples/e15.hpp"
#include "examples/e13.hpp"
#include "examples/e16.hpp"
#include "examples/e17.hpp"
#include "examples/e18.hpp"
#include "examples/e19.hpp"
#include "examples/e20.hpp"
#include "examples/e21.hpp"
#include "s3/IP.hpp"

void run_code () {
#ifdef __linux__
    e18();
#else
    /* Host */
    s3_IP_Host host {9001};

    if (!host.listen_and_accept() && !host.is_connected()) {
        std::cerr << "ERROR: [main] Host was unable to accept client.\n";
        return;
    }

    int64_t data;
    while (true) {
        if (host.Receive((void*)(&data)) <= 0) {
            std::cerr <<"ERROR: [main] Improper data handling.\n";
            return;
        }
        else {
            std::cout << "INFO: [main] Received: " << data << '\n';
        }

        if (data < 1) {
            return;
        }
    }

#endif
}

int main () {
    run_code ();
}
