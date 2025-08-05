#include "IP.hpp"
#include <cstring> // for memset

// -----------------------
// s3_IP_Packet
// -----------------------
char &s3_IP_Packet::operator[](size_t index) {
    size_t data_index = index + header_length; // Adjust index to account for header length

    if (data_index >= received) {
        throw std::out_of_range("Index out of range in s3_IP_Packet");
    }
    return data[data_index];
}


// -----------------------
// s3_Communication_Handler
// -----------------------
s3_Communication_Handler::s3_Communication_Handler() {
    m_header_size = sizeof(FieldInfo);
}

s3_Communication_Handler::~s3_Communication_Handler() {
    if (m_commit_data) {
        delete[] m_commit_data;
    }
}

void s3_Communication_Handler::CommitData(size_t size) {
    if (m_commit_data) delete[] m_commit_data;

    m_commit_size = size + m_header_size;
    m_commit_data = new char[m_commit_size];
    memset(m_commit_data, 0, m_commit_size);
}

std::pair<size_t, char*> s3_Communication_Handler::AddHeader(char *data, size_t size) {
    if (!m_commit_data || size + m_header_size > m_commit_size) {
        throw std::runtime_error("Commit data not initialized or size exceeds limit.");
    }

    // Construct header
    FieldInfo info {
        .length = static_cast<uint32_t>(size),
        .id = m_id++
    };

    // Copy header into header section
    std::memcpy(m_commit_data, &info, sizeof(FieldInfo));
    
    // Copy the actual data after the header
    std::memcpy(m_commit_data + m_header_size, data, size);

    return {static_cast<size_t>(size + m_header_size), m_commit_data}; // Return the pointer to the commit data
}

s3_Communication_Handler::FieldInfo s3_Communication_Handler::GetHeader(char *data) {
    FieldInfo info;
    std::memcpy(&info, data, sizeof(FieldInfo));
    return info;
}



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

    m_handler.CommitData(length);
    auto [packet_size, packet] = m_handler.AddHeader(static_cast<char*>(data), length);


    ssize_t sent = send(m_socket_fd, packet, packet_size, 0);
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

    char* buf = reinterpret_cast<char*>(buffer);
    size_t total_received = 0;

    // The first packet should *always* contain the header
    std::cout<<"INFO: [s3_IP_HOST] Buffer Pointer: "<<static_cast<void*>(buf)<<"\n";
    std::cout<<"INFO: [s3_IP_HOST] Buffer Length: "<<length<<"\n";
    ssize_t received = recv(m_client_fd, buf, length, 0);
    // Read the first buffer to get the header
    auto field_info = m_handler.GetHeader(buf); 

    ssize_t total_read_required = sizeof(field_info) + field_info.length;
    ssize_t remaining_data = total_read_required - received;
    std::cout<<"INFO: [s3_IP_HOST] Total Read Required: "<<total_read_required<<"\n";
    std::cout<<"INFO: [s3_IP_HOST] Remaining Data: "<<remaining_data<<"\n";
    
    // read until remaining data is received
    while (remaining_data > 0) {
        ssize_t chunk_size = recv(m_client_fd, buf + received, remaining_data, 0);
        if (chunk_size < 0) {
            std::cerr << "Failed to receive data\n";
            return -1;
        }
        if (chunk_size == 0) {
            // Connection closed or no more data
            break;
        }
        received += chunk_size;
        remaining_data -= chunk_size;
    }
    //buf += sizeof(field_info); // Move buffer pointer past header
    return static_cast<int>(received);
}

s3_IP_Packet s3_IP_Host::Receive(s3_IP_Packet packet) {
    int received = Receive(packet.data, packet.length);
    s3_IP_Packet p {
        .data = packet.data,
        .length = packet.length,
        .received = static_cast<size_t>(received),.header_length = sizeof(s3_Communication_Handler::FieldInfo)
    };
    return p;
}


bool s3_IP_Host::is_connected() const {
    return m_connected;
}