#include "apppac.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>   // ntohl on Linux/macOS

AppPacket::AppPacket(_AppPacType type) {
    m_type     = type;
    m_success  = false;
    m_length   = 0;
    m_frame_id = 0;
    std::fill(std::begin(m_data), std::end(m_data), 0);
}

AppPacket::~AppPacket() {
    // No dynamic memory to clean up
}

void AppPacket::Set_Success(bool success) {
    m_success = success;
}

void AppPacket::Set_Data(const std::vector<uint32_t>& values) {
    m_length = std::min<size_t>(values.size(), 24);
    for (size_t i = 0; i < m_length; ++i) {
        m_data[i] = values[i];
    }
}

void AppPacket::Set_Help(const std::string& help_message) {
    if (help_message.size() > 96) {
        m_help_message = help_message.substr(0, 96); // Truncate if too long
    } 
    else {
        m_help_message = help_message;
    }
}

void AppPacket::Set_Command(const std::string& command) {
    if (command.size() > 96) {
        m_command = command.substr(0, 96); // Truncate if too long
    } 
    else {
        m_command = command;
    }
}

void AppPacket::Set_FrameID(uint32_t frame_id) {
    m_frame_id = frame_id;
}

_AppPac AppPacket::Create() const {
    _AppPac packet{};
    packet.type     = m_type;
    packet.frame_id = m_frame_id;

    switch (packet.type) {
        case SET:
            packet.length = 1;
            packet.success = m_success;
            break;
        case GET:
        case DATA:
            packet.length = m_length;
            std::copy(std::begin(m_data), std::begin(m_data) + m_length, std::begin(packet.data));
            break;
        case HELP:
            // Encode to help_message as bytes
            packet.length = (m_help_message.size() + 3) / 4; // Round up to nearest 4 bytes
            std::fill(std::begin(packet.help_message), std::end(packet.help_message), 0); // Clear help_message buffer
            std::copy(m_help_message.begin(), m_help_message.end(), packet.help_message);
            break;
        case CMD:
            packet.length = (m_command.size() + 3) / 4; // Round up to nearest 4 bytes
            std::fill(std::begin(packet.command), std::end(packet.command), 0);
            std::copy(m_command.begin(), m_command.end(), packet.command);
            break;
        default:
            packet.length = 0;
            break;
    }
    return packet;
}

_AppPac AppPacket::Interpret(const uint8_t* raw_data, size_t raw_length)
{
    //  Wire layout (network byte order where applicable):
    //  [0..3]    type      (uint32_t)
    //  [4..7]    length    (uint32_t)
    //  [8..103]  payload   (96 bytes)
    //  [104..107] frame_id (uint32_t)
    constexpr size_t TYPE_OFFSET     = 0;
    constexpr size_t LENGTH_OFFSET   = 4;
    constexpr size_t PAYLOAD_OFFSET  = 8;
    constexpr size_t PAYLOAD_SIZE    = 96;   // 24 * sizeof(uint32_t)
    constexpr size_t FRAME_ID_OFFSET = 104;
    constexpr size_t WIRE_SIZE       = 108;  // FRAME_ID_OFFSET + sizeof(uint32_t)

    if (raw_data == nullptr) {
        throw std::invalid_argument("raw_data is null");
    }

    if (raw_length < WIRE_SIZE) {
        throw std::runtime_error(
            "raw_data too small: need " + std::to_string(WIRE_SIZE) +
            " bytes, got " + std::to_string(raw_length));
    }

    _AppPac packet{};

    // ---- header ----
    uint32_t type_net, length_net, frame_id_net;
    std::memcpy(&type_net,     raw_data + TYPE_OFFSET,     4);
    std::memcpy(&length_net,   raw_data + LENGTH_OFFSET,   4);
    std::memcpy(&frame_id_net, raw_data + FRAME_ID_OFFSET, 4);

    packet.type     = static_cast<_AppPacType>(ntohl(type_net));
    packet.length   = ntohl(length_net);
    packet.frame_id = ntohl(frame_id_net);

    // ---- payload ----
    const uint8_t* payload = raw_data + PAYLOAD_OFFSET;

    switch (packet.type) {
        case SET:
            packet.success    = payload[0] != 0;
            packet.padding[0] = 0;
            packet.padding[1] = 0;
            packet.padding[2] = 0;
            break;

        case GET:
        case DATA: {
            size_t count = std::min<size_t>(packet.length, 24);
            for (size_t i = 0; i < count; ++i) {
                uint32_t word_net;
                std::memcpy(&word_net, payload + i * 4, 4);
                packet.data[i] = ntohl(word_net);
            }
            break;
        }

        case HELP:
            std::memcpy(packet.help_message, payload, PAYLOAD_SIZE);
            break;

        case CMD:
            std::memcpy(packet.command, payload, PAYLOAD_SIZE);
            break;

        default:
            throw std::runtime_error(
                "invalid _AppPac type: " + std::to_string(static_cast<uint32_t>(packet.type)));
    }

    return packet;
}

AppPoint::AppPoint() {
    // Nothing to initialize
}

AppPoint::~AppPoint() {
    // Nothing to clean up
}

void AppPoint::Listen(const std::string& ip, uint16_t port) {
    m_endpoint.Listen(ip, port);
}

void AppPoint::Connect(const std::string& ip, uint16_t port) {
    m_endpoint.Connect(ip, port);
}

void AppPoint::Send(const _AppPac &raw_packet) {
    //  Wire layout matches Interpret:
    //  [0..3]    type
    //  [4..7]    length
    //  [8..103]  payload (96 bytes)
    //  [104..107] frame_id
    constexpr size_t kWireSize    = 108;
    constexpr size_t kPayloadSize = 96;
    std::vector<uint8_t> wire(kWireSize, 0);

    const uint32_t type_net     = htonl(static_cast<uint32_t>(raw_packet.type));
    const uint32_t length_net   = htonl(raw_packet.length);
    const uint32_t frame_id_net = htonl(raw_packet.frame_id);

    std::memcpy(wire.data(),       &type_net,     4);
    std::memcpy(wire.data() + 4,   &length_net,   4);
    std::memcpy(wire.data() + 104, &frame_id_net, 4);

    uint8_t* payload = wire.data() + 8;
    switch (raw_packet.type) {
        case SET:
            payload[0] = raw_packet.success ? 1 : 0;
            break;

        case GET:
        case DATA: {
            size_t count = std::min<size_t>(raw_packet.length, 24);
            for (size_t i = 0; i < count; ++i) {
                const uint32_t word_net = htonl(raw_packet.data[i]);
                std::memcpy(payload + i * 4, &word_net, 4);
            }
            break;
        }

        case HELP:
            std::memcpy(payload, raw_packet.help_message, kPayloadSize);
            break;

        case CMD:
            std::memcpy(payload, raw_packet.command, kPayloadSize);
            break;

        default:
            break;
    }

    m_endpoint.Send(wire);
}

void AppPoint::Send(const AppPacket& packet) {
    _AppPac raw_packet = packet.Create();
    Send(raw_packet);
}

std::vector<_AppPac> AppPoint::Receive() {
    std::vector<_AppPac> packets;
    std::vector<uint8_t> raw_data = m_endpoint.Receive();
    if (!raw_data.empty()) {
        m_rx_buffer.insert(m_rx_buffer.end(), raw_data.begin(), raw_data.end());
    }

    constexpr size_t kPacketSize = 108;  // must match Interpret / Send wire size
    AppPacket parser(SET);

    while (m_rx_buffer.size() >= kPacketSize) {
        try {
            _AppPac raw_packet = parser.Interpret(m_rx_buffer.data(), kPacketSize);

            packets.push_back(raw_packet);
            m_rx_buffer.erase(m_rx_buffer.begin(), m_rx_buffer.begin() + kPacketSize);
        } catch (const std::exception& e) {
            std::cerr << "Error interpreting received data: " << e.what() << std::endl;
            m_rx_buffer.erase(m_rx_buffer.begin(), m_rx_buffer.begin() + kPacketSize);
        }
    }

    return packets;
}