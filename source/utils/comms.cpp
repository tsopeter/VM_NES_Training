#include "comms.hpp"


size_t CommsNode::GetSize() {
    switch (type) {
        case COMMS_CLIENT:
        case COMMS_HOST:
        case COMMS_UNKNOWN_TYPE:
            return 0;
        
        case COMMS_INT:
            return sizeof(int) + 1; // 1 byte for type + sizeof(int)
        case COMMS_INT64:
            return sizeof(int64_t) + 1; // 1 byte for type + sizeof(int64_t)
        case COMMS_DOUBLE:
            return sizeof(double) + 1; // 1 byte for type + sizeof(double)
        case COMMS_STRING:
            return std::strlen(data) + 1 + 1; // 1 byte for type + strlen(data)
        case COMMS_IMAGE: {
            // Assuming data is a pointer to a tensor, we need to know the size of the tensor
            auto *tensor = reinterpret_cast<torch::Tensor*>(data);
            int64_t Height = tensor->size(0);
            int64_t Width = tensor->size(1);

            // Single channel image, so we need to account for the size of the tensor
            return (Height * Width) + 2 * sizeof(size_t) + 1; // 1 byte for type + 2 * sizeof(size_t) for Height and Width
        }
        default:
            return 0;
    }
}

CommsDataPacket::CommsDataPacket() {
    for (size_t i = 0; i < max_size; ++i) {
        nodes[i].type = COMMS_UNKNOWN_TYPE;
        nodes[i].data = nullptr;
    }
}

CommsNode &CommsDataPacket::operator[](size_t index) {
    if (index < max_size) {
        return nodes[index];
    }
    throw std::out_of_range("Index out of range");
}

char *CommsDataPacket::CreatePacket(size_t &size) {
    size = 0;
    // Iterate through the nodes and calculate the required size
    for (size_t i = 0; i < max_size; ++i) {
        size += nodes[i].GetSize();
    }
    std::cout << "INFO: [CommsDataPacket] Total size required for packet: " << size << " bytes.\n";

    // allocate memory for the packet
    char *packet = new char[size + 1];
    size += 1; // +1 for the type byte

    // Populate the packet with data from nodes
    size_t offset = 0;
    packet[0] = COMMS_DP;
    offset += 1; // Move past the type byte

    for (size_t i = 0; i < max_size; ++i) {
        if (nodes[i].type == COMMS_UNKNOWN_TYPE)
            continue; // Skip nodes that are not set


        packet[offset] = static_cast<char>(nodes[i].type);
        offset += 1; // Move past the type byte
        switch (nodes[i].type) {
            case COMMS_INT: {
                std::cout<<"INFO: [CommsDataPacket] Writing INT data to packet.\n";
                std::memcpy(packet + offset, nodes[i].data, sizeof(int));
                offset += sizeof(int);
                break;
            }
            case COMMS_INT64: {
                std::cout<<"INFO: [CommsDataPacket] Writing INT64 data to packet.\n";
                std::memcpy(packet + offset, nodes[i].data, sizeof(int64_t));
                offset += sizeof(int64_t);
                break;
            }
            case COMMS_DOUBLE: {
                std::cout<<"INFO: [CommsDataPacket] Writing DOUBLE data to packet.\n";
                std::memcpy(packet + offset, nodes[i].data, sizeof(double));
                offset += sizeof(double);
                break;
            }
            case COMMS_STRING: {
                std::cout<<"INFO: [CommsDataPacket] Writing STRING data to packet.\n";
                size_t str_length = std::strlen(nodes[i].data);
                std::memcpy(packet + offset, nodes[i].data, str_length);
                offset += str_length;
                break;
            }
            case COMMS_IMAGE: {
                std::cout<<"INFO: [CommsDataPacket] Writing IMAGE data to packet.\n";
                auto *tensor = reinterpret_cast<torch::Tensor*>(nodes[i].data);
                int64_t Height = tensor->size(0);
                int64_t Width = tensor->size(1);
                std::memcpy(packet + offset, &Height, sizeof(size_t));
                offset += sizeof(size_t);
                std::memcpy(packet + offset, &Width, sizeof(size_t));
                offset += sizeof(size_t);
                std::memcpy(packet + offset, tensor->data_ptr<uint8_t>(), Height * Width);
                offset += Height * Width;
                break;
            }
        }
    }
    std::cout<<"INFO: [CommsDataPacket] Created packet of size "<<size<<" bytes.\n";
    return packet;
}




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

    image = image.to(torch::kCPU).contiguous().to(torch::kUInt8);

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

void Comms::TransmitDataPacket(CommsDataPacket &packet) {
    if (m_connection_type != COMMS_CLIENT) {
        throw std::runtime_error("Comms is not a client.");
    }

    size_t size = 0;
    char *data = packet.CreatePacket(size);
    if (size == 0) {
        std::cerr << "ERROR: [Comms] No data to transmit.\n";
        delete[] data;
        return;
    }

    client->Transmit(data, size);
    delete[] data; // Clean up the allocated memory

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
    if (type == COMMS_DP) {
        std::cout<<"INFO: [Comms] Received data packet of type COMMS_DP.\n";
        m_dp_offset = 0; // Reset offset for data packet reading
    
        std::cout<<"INFO: [Comms] Data packet packet pointer: "<<static_cast<void*>(&m_staging_packet[0])<<"\n";
        std::cout<<"INFO: [Comms] Data packet received with size: "<<m_staging_packet.received<<" bytes.\n"; 
    }
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

CommsType Comms::ReceiveDataPacket() {
    if (m_connection_type != COMMS_HOST) {
        throw std::runtime_error("Comms is not a host.");
    }

    // Read the data in the staging buffer (read by Receive())
    s3_IP_Packet packet = m_staging_packet;

    if (packet.received <= 0) {
        throw std::runtime_error("Failed to receive data packet.");
    }

    CommsType type = static_cast<CommsType>(packet[0]);
    if (type != COMMS_DP) {
        throw std::runtime_error("Received data packet is not of type COMMS_DP.");
    }
    m_dp_offset += 1;

    // Read the first field of the CommsDataPacket
    CommsType field_0 = static_cast<CommsType>(packet[m_dp_offset]);
    m_dp_offset += 1; // Move to the next field
    return field_0;
}

CommsType Comms::DP_ReadField() {
    CommsType type = static_cast<CommsType>(m_staging_packet[m_dp_offset]);
    m_dp_offset += 1; // Move to the next field
    return type;
}

double Comms::DP_ReceiveDouble() {
    // Read the double value from the packet
    double value;
    std::memcpy(&value, &(m_staging_packet[m_dp_offset]), sizeof(double));
    m_dp_offset += sizeof(double);  // move past the double value
    return value;
}

int Comms::DP_ReceiveInt() {
    // Read the int value from the packet
    int value; 

    char *pointer = &(m_staging_packet[m_dp_offset]);
    std::cout<<"INFO: [Comms] DP_ReceiveInt pointer: "<<static_cast<void*>(pointer)<<"\n";
    std::memcpy(&value, pointer, sizeof(int));
    m_dp_offset += sizeof(int);  // move past the int value
    return value;
}

int64_t Comms::DP_ReceiveInt64() {
    // Read the int64_t value from the packet
    int64_t value;
    std::memcpy(&value, &(m_staging_packet[m_dp_offset]), sizeof(int64_t));
    m_dp_offset += sizeof(int64_t);  // move past the int64_t value
    return value;
}

Texture Comms::DP_ReceiveImageAsTexture() {
    // Read the image data from the packet
    char *staging_buffer = &(m_staging_packet[m_dp_offset]);
    size_t Height, Width;
    std::memcpy(&Height, staging_buffer, sizeof(size_t));
    std::memcpy(&Width, staging_buffer + sizeof(size_t), sizeof(size_t));
    size_t size = Height * Width * sizeof(uint8_t);

    std::cout<<"INFO: [Comms] Offset: " << m_dp_offset << ", Height: " << Height << ", Width: " << Width << ", Size: " << size << "\n";
    std::cout<<"INFO: [Comms] Expected offset after: " << m_dp_offset + size + 2 * sizeof(size_t) << "\n";

    Image image {
        .data = staging_buffer + 2 * sizeof(size_t),
        .width = static_cast<int>(Width),
        .height = static_cast<int>(Height),
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
    };

    std::cout<<"INFO: [Comms] Image height: " << image.height << ", width: " << image.width << ", size: " << size << "\n";

    // Create texture from image
    Texture texture = LoadTextureFromImage(image);
    m_dp_offset += size + 2 * sizeof(size_t); // Move past the image data
    return texture;
}