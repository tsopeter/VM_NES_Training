#include <iostream>
#include "../s3/Serial.hpp"

int e7 () {
    std::cout<<
        "Serial Example...\n"
        "Press 1 to signal, and 0 to exit\n"
        "Look at the FPGA to see if serial is sent.\n";

    int x;
    Serial serial {"/dev/tty.usbmodem8326898B1E1E1", 115200};
    serial.Open();
    while (true) {
        std::cin>>x;
        if (x == 1)
            serial.Signal();

        if (x == 0)
            break;
    }
    serial.Close();
    return 0;
}