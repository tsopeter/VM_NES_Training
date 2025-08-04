#include "IP.hpp"
#include <cstring> // for memset

// -----------------------
// s3_IP_Client
// -----------------------

s3_IP_Client::s3_IP_Client(const std::string& ip, uint16_t port)
    : m_ip(ip), m_port(port), m_connected(false), m_socket_fd(-1) {}

s3_IP_Client::~s3_IP_Client() {
    disconnect();
}

bool s3_IP_Client::connect() {
    m_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket_fd < 0) {
        std::cerr << "Socket creation failed\n";
        return false;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(m_port);

    if (inet_pton(AF_INET, m_ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address or address not supported\n";
        return false;
    }

    if (::connect(m_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection to " << m_ip << ":" << m_port << " failed\n";
        close(m_socket_fd);
        m_socket_fd = -1;
        return false;
    }

    m_connected = true;
    return true;
}

void s3_IP_Client::disconnect() {
    if (m_connected && m_socket_fd != -1) {
        close(m_socket_fd);
        m_socket_fd = -1;
        m_connected = false;
    }
}

void s3_IP_Client::Transmit(void* data, size_t length) {
    if (!m_connected || m_socket_fd == -1) return;

    ssize_t sent = send(m_socket_fd, data, length, 0);
    if (sent < 0) {
        std::cerr << "Failed to send data\n";
    }
}

bool s3_IP_Client::is_connected() const {
    return m_connected;
}

int s3_IP_Client::Receive(void* buffer) {
    if (!m_connected || m_socket_fd == -1) return -1;

    constexpr size_t MAX_RECV_SIZE = 1024;
    int received = recv(m_socket_fd, buffer, MAX_RECV_SIZE, 0);
    if (received < 0) {
        std::cerr << "Failed to receive data\n";
    }
    return received;
}

// -----------------------
// s3_IP_Host
// -----------------------

s3_IP_Host::s3_IP_Host(uint16_t port)
    : m_port(port), m_connected(false), m_server_fd(-1), m_client_fd(-1) {}

s3_IP_Host::~s3_IP_Host() {
    disconnect();
}

bool s3_IP_Host::listen_and_accept() {
    m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_fd < 0) {
        std::cerr << "Server socket creation failed\n";
        return false;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(m_port);

    if (bind(m_server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind failed\n";
        return false;
    }

    if (listen(m_server_fd, 1) < 0) {
        std::cerr << "Listen failed\n";
        return false;
    }

    m_client_fd = accept(m_server_fd, nullptr, nullptr);
    if (m_client_fd < 0) {
        std::cerr << "Accept failed\n";
        return false;
    }

    m_connected = true;
    return true;
}

void s3_IP_Host::disconnect() {
    if (m_client_fd != -1) {
        close(m_client_fd);
        m_client_fd = -1;
    }
    if (m_server_fd != -1) {
        close(m_server_fd);
        m_server_fd = -1;
    }
    m_connected = false;
}

int s3_IP_Host::Receive(void* buffer, size_t length) {
    if (!m_connected || m_client_fd == -1) return -1;

    size_t total_received = 0;
    char* buf = reinterpret_cast<char*>(buffer);

    while (total_received < length) {
        ssize_t received = recv(m_client_fd, buf + total_received, length - total_received, 0);
        if (received <= 0) {
            std::cerr << "Failed to receive full data\n";
            return -1;
        }
        // Exit if we received all the data, i.e., no more data to read
        if (received == 0) {
            break;
        }
        total_received += received;
    }

    return static_cast<int>(total_received);
}

/*
int s3_IP_Host::Receive(void* buffer) {
    if (!m_connected || m_client_fd == -1) return -1;

    constexpr size_t MAX_RECV_SIZE = 1024;
    int received = recv(m_client_fd, buffer, MAX_RECV_SIZE, 0);
    if (received < 0) {
        std::cerr << "Receive failed\n";
    }
    return received;
}
*/

bool s3_IP_Host::is_connected() const {
    return m_connected;
}