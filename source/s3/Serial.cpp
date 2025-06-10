//
//  Serial.cpp
//  ONN-Model
//
//  Created by Peter Tso on 11/8/24.
//

#include "Serial.hpp"

#include <stdexcept>
#include <fcntl.h>      // File control definitions
#include <unistd.h>     // UNIX standard function definitions

Serial::Serial (const std::string port_name, int baud_rate)
: port_name(port_name), baud_rate(baud_rate) {}

Serial::Serial (const char port_name[], int baud_rate)
: port_name(port_name), baud_rate(baud_rate) {}

Serial::~Serial() {
    Close();
}

void Serial::Open () {
    port = open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

    if (port < 0)
        throw std::runtime_error(std::string{"Error opening serial port"} + port_name +
                                 std::string{"Errno: "} + std::string{strerror(errno)});
    fcntl(port, F_SETFL, 0); // Clear O_NONBLOCK
    
    struct termios options;
    tcgetattr(port, &options);

    // Set baud rate
    cfsetispeed(&options, baud_rate);
    cfsetospeed(&options, baud_rate);

    // Set data bits, stop bits, and parity
    options.c_cflag &= ~PARENB; // No parity
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE;  // Clear data size
    options.c_cflag |= CS8;     // 8 data bits

    options.c_cflag |= (CLOCAL | CREAD); // Enable receiver, set local mode
    options.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable flow control
    
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input
    options.c_oflag &= ~OPOST; // Raw output

    tcsetattr(port, TCSANOW, &options);
    
}

void Serial::Close() {
    if (port >= 0) {
        close(port);
        port = -1;
    }
}

int Serial::Send(const std::string & data) {
    ssize_t num_bytes_written = write(port, data.c_str(), data.size());
    if (num_bytes_written <= 0) {
        return -1;
    }
    return static_cast<int>(num_bytes_written);
}

int Serial::Send(const char *data) {
    const std::string dat {data};
    return Send(dat);
}

void Serial::Signal() {
    // Faster than Send as we skip building a string altogether...
    // Also, we kinda just assume that Signal always works...
    // We can send a test using Send(...) to make sure tho
    static const char newline[] = "\n";
    write(port, newline, 1);
}
