#include "comms.hpp"

Comms::Comms(CommsType type) : m_connection_type(type) {
    if (type == COMMS_CLIENT) {
        client = new s3_IP_Client("", 0);
    } else if (type == COMMS_HOST) {
        host = new s3_IP_Host(0); 

        // set up the staging buffer
        m_staging_buffer = new char[m_staging_buffer_size];
        std::cout<<"INFO: [Comms] Pointer @ "<<static_cast<void*>(m_staging_buffer)<<"\n";
        if (!m_staging_buffer) {
            throw std::runtime_error("Failed to allocate staging buffer.");
        }
        std::cout<<"INFO: [Comms] Staging buffer of size "<<m_staging_buffer_size<<" bytes allocated.\n";
        ResetStagingPacket();
    }
}

void Comms::Connect() {
    if (m_connection_type == COMMS_CLIENT) {
        while (!client->is_connected()) {
            client->connect();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }  
    }
    else {
        while (!host->is_connected()) {
            host->listen_and_accept();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void Comms::SetParameters(int port, const std::string ip) {
    if (client) {
        client->m_port = port;
        client->m_ip = ip;
    } else if (host) {
        host->m_port = port;
    }
}

void Comms::ResetStagingPacket() {
    m_staging_packet.data = m_staging_buffer;
    m_staging_packet.length = m_staging_buffer_size;
    m_staging_packet.received = 0;
}

Comms::~Comms() {
    if (client) {
        delete client;
    }
    if (host) {
        delete host;
        delete[] m_staging_buffer;
    }
}

// Transmit methods (client only )

void Comms::Transmit (char *p_data, size_t size) {
    // Transmit data over the network
    if (m_connection_type != COMMS_CLIENT) {
        throw std::runtime_error("Comms is not a client.");
    }

    char data[size + 1];
    data[0] = COMMS_UNKNOWN_TYPE;
    std::memcpy(data + 1, p_data, size);
    client->Transmit(data, sizeof(data));
}

void Comms::TransmitDouble(double value) {
    if (m_connection_type != COMMS_CLIENT) {
        throw std::runtime_error("Comms is not a client.");
    }

    // Set data packet (first byte is type)
    char data[sizeof(double)+ 1];
    data[0] = COMMS_DOUBLE; // First byte indicates type
    std::memcpy(data + 1, &value, sizeof(double));  
    client->Transmit(data, sizeof(data));
}

void Comms::TransmitInt64(int64_t value) {
    if (m_connection_type != COMMS_CLIENT) {
        throw std::runtime_error("Comms is not a client.");
    }

    // Set data packet (first byte is type)
    char data[sizeof(int64_t) + 1];
    data[0] = COMMS_INT64; // First byte indicates type
    std::memcpy(data + 1, &value, sizeof(int64_t));
    client->Transmit(data, sizeof(data));
}

void Comms::TransmitInt(int value) {
    if (m_connection_type != COMMS_CLIENT) {
        throw std::runtime_error("Comms is not a client.");
    }
    // Set data packet (first byte is type)
    char data[sizeof(int) + 1];
    data[0] = COMMS_INT; // First byte indicates type
    std::memcpy(data + 1, &value, sizeof(int));
    client->Transmit(data, sizeof(data));   
}

void Comms::TransmitImage(torch::Tensor image) {
    if (m_connection_type != COMMS_CLIENT) {
        throw std::runtime_error("Comms is not a client.");
    }
    if (image.device() != torch::kCPU)  // Ensure image is on CPU
        image = image.to(torch::kCPU).contiguous();

    // Set data packet (first byte is type)
    // Get the size of image, assume tensor is uint8_t of H,W
    size_t size   = image.numel() * sizeof(uint8_t);
    size_t Height = image.size(0);
    size_t Width  = image.size(1);
    char data[size + 1 + 2 * sizeof(size_t)];
    data[0] = COMMS_IMAGE;
    // Copy the size and dimensions into the data packet
    std::memcpy(data + 1, &Height, sizeof(size_t));
    std::memcpy(data + 1 + sizeof(size_t), &Width, sizeof(size_t));

    // Copy the image data into the data packet
    std::memcpy(data + 1 + 2 * sizeof(size_t), image.data_ptr<uint8_t>(), size);
    client->Transmit(data, sizeof(data));
}

void Comms::TransmitDisconnect() {
    if (m_connection_type != COMMS_CLIENT) {
        throw std::runtime_error("Comms is not a client.");
    }

    char data[1];
    data[0] = COMMS_DISCONNECT; // First byte indicates disconnect
    client->Transmit(data, sizeof(data));
}

void Comms::TransmitString(const std::string &str) {
    if (m_connection_type != COMMS_CLIENT) {
        throw std::runtime_error("Comms is not a client.");
    }

    size_t size = str.size();
    char data[size + 1];
    data[0] = COMMS_STRING; // First byte indicates type
    std::memcpy(data + 1, str.data(), size);

    std::cout<<"INFO: [Comms] Transmitting string of size "<<size<<" bytes.\n";
    client->Transmit(data, sizeof(data));
}

// Receive methods (host only)
CommsType Comms::Receive() {
    if (m_connection_type != COMMS_HOST) {
        throw std::runtime_error("Comms is not a host.");
    }

    // Receive data packet into staging buffer
    s3_IP_Packet packet {
        .data = m_staging_buffer,
        .length = m_staging_buffer_size,
        .received = 0,
        .header_length = sizeof(s3_Communication_Handler::FieldInfo)
    };

    std::cout<<"INFO: [Comms] Waiting to receive data...\n";
    m_staging_packet = host->Receive(packet); 

    if (m_staging_packet.received <= 0) {
        throw std::runtime_error("Failed to receive data.");
    }

    CommsType type = static_cast<CommsType>(m_staging_packet[0]);
    return type;
}

double Comms::ReceiveDouble() {
    if (m_connection_type != COMMS_HOST) {
        throw std::runtime_error("Comms is not a host.");
    }
    double value;
    std::memcpy(&value, &(m_staging_packet[0])+1, sizeof(double));
    return value;
}

int Comms::ReceiveInt() {
    if (m_connection_type != COMMS_HOST) {
        throw std::runtime_error("Comms is not a host.");
    }

    // Read the data in the staging buffer (read by Receive())
    int value;
    std::memcpy(&value, &(m_staging_packet[0])+1, sizeof(int));
    return value;
}

int64_t Comms::ReceiveInt64() {
    if (m_connection_type != COMMS_HOST) {
        throw std::runtime_error("Comms is not a host.");
    }

    // Read the data in the staging buffer (read by Receive())
    int64_t value;
    std::memcpy(&value, &(m_staging_packet[0])+1, sizeof(int64_t));
    return value;
}

Texture Comms::ReceiveImageAsTexture() {
    if (m_connection_type != COMMS_HOST) {
        throw std::runtime_error("Comms is not a host.");
    }

    char *staging_buffer = &(m_staging_packet[0])+1;
    size_t Height, Width;
    std::memcpy(&Height, staging_buffer, sizeof(size_t));
    std::memcpy(&Width, staging_buffer + sizeof(size_t), sizeof(size_t));

    size_t size = Height * Width * sizeof(uint8_t);
    Image image {
        .data = staging_buffer + 2 * sizeof(size_t),
        .width = static_cast<int>(Width),
        .height = static_cast<int>(Height),
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
    };

    // Create texture from image
    Texture texture = LoadTextureFromImage(image);
    return texture;
}

torch::Tensor Comms::ReceiveImage() {
    if (m_connection_type != COMMS_HOST) {
        throw std::runtime_error("Comms is not a host.");
    }

    // Read the data in the staging buffer (read by Receive())
    char *staging_buffer = &(m_staging_packet[0]) + 1;
    size_t Height, Width;
    std::memcpy(&Height, staging_buffer, sizeof(size_t));
    std::memcpy(&Width, staging_buffer + sizeof(size_t), sizeof(size_t));

    size_t size = Height * Width * sizeof(uint8_t);
    auto image_tensor = torch::from_blob(
        staging_buffer + 2 * sizeof(size_t),
        {static_cast<int64_t>(Height), 
         static_cast<int64_t>(Width)},
        torch::kUInt8
    ).clone(); // Clone to ensure tensor owns its data

    return image_tensor;
}

std::string Comms::ReceiveString() {
    if (m_connection_type != COMMS_HOST) {
        throw std::runtime_error("Comms is not a host.");
    }

    // Read the data in the staging buffer (read by Receive())
    char *staging_buffer = &(m_staging_packet[0]) + 1;
    size_t size = m_staging_packet.received - 1 - m_staging_packet.header_length; // Exclude type byte and header

    std::string str(staging_buffer, size);
    return str;
}