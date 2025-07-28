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
#include "utils/utils.hpp"

std::ostream& operator<<(std::ostream &os, const Utils::data_structure &ds) {
    return os << "Step: " << ds.iteration << ", Total Rewards: " << ds.total_rewards << '\n';
}

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

    Utils::data_structure ds;
    while (true) {
        if (host.Receive((void*)(&ds)) <= 0) {
            std::cerr <<"ERROR: [main] Improper data handling.\n";
            return;
        }
        else {
            std::cout << "INFO: [main] Received:\n" << ds << '\n';
        }

        if (ds.iteration < 1) {
            return;
        }
    }

#endif
}

int main () {
    run_code ();
}
