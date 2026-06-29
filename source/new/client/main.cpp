#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unistd.h>

#include "client.hpp"
#include "common/apppac.hpp"

#ifdef  SERIAL
#define _SERIAL_CALLS_ONLY
#endif

#define _APP_IMPLT_DIRZ_NOTHING_GET                 \
    []() -> std::vector<uint32_t> { return {}; }    \

#define _APP_IMPLT_DIRZ(cmd, func_1, func_2, help, ap)                                      \
    switch (cmd.direction) {                                                                \
        case _SET: {                                                                        \
            AppPacket packet(_AppPacType::SET);                                             \
            func_1(cmd.parameter);                                                          \
            packet.Set_Success(true);                                                       \
            ap.Send(packet);                                                                \
        } break;                                                                            \
        case _GET: {                                                                        \
            AppPacket packet(_AppPacType::GET);                                             \
            auto x = func_2();                                                              \
            packet.Set_Data({x});                                                             \
            ap.Send(packet);                                                                \
        } break;                                                                            \
        case _HELP: {                                                                       \
            AppPacket packet(_AppPacType::HELP);                                            \
            packet.Set_Help(help);                                                          \
            ap.Send(packet);                                                                \
        } break;                                                                            \
        default: {                                                                          \
            std::cerr << "Unknown command direction: " << cmd.direction << std::endl;       \
        }                                                                                   \
    } break;                                                                                \

enum _CommandType {
    // Directions
    _SET  = 1,
    _GET  = 2,
    _HELP = 3,

    // Types
    _HEARTBEAT   = 100,
    _ID          = 101,
    _RESET       = 102,
    _BURST_LIMIT = 103,
    _START       = 104,
    _PERSIST     = 105,
    _PERIOD      = 106,
    _WIDTH       = 107,
    _PULSES      = 108,
    _FRAME_COUNT = 109,
    _WRITES      = 110,
    _N_AVG       = 111,
    _DELAY       = 112,
    _MEAN_ENABLE = 113,
    _IRQ_ENABLE  = 114,
    _MEM_BUF_SEL = 115,
    _MEM_MODES   = 116,
    _IRQ_FC      = 117,
    _FRAME       = 118,
    _FRAME2      = 119,


    // Commands
    _TRIGGER     = 200,
    _READ_DATA   = 201,
    _DISCONNECT  = 202,
    _EXIT        = 203,
    _TRIG_CONT   = 204
};

struct _Command {
    _CommandType direction;
    _CommandType type;
    uint32_t     parameter; // Optional

    void Print () {
        static const std::unordered_map<_CommandType, std::string> kDirectionMap = {
            {_SET, "SET"},
            {_GET, "GET"},
            {_HELP, "HELP"}
        };

        static const std::unordered_map<_CommandType, std::string> kTypeMap = {
            {_HEARTBEAT, "HEARTBEAT"},
            {_ID, "ID"},
            {_RESET, "RESET"},
            {_BURST_LIMIT, "BURST_LIMIT"},
            {_START, "START"},
            {_PERSIST, "PERSIST"},
            {_PERIOD, "PERIOD"},
            {_WIDTH, "WIDTH"},
            {_PULSES, "PULSES"},
            {_FRAME_COUNT, "FRAME_COUNT"},
            {_WRITES, "WRITES"},
            {_N_AVG, "N_AVG"},
            {_DELAY, "DELAY"},
            {_MEAN_ENABLE, "MEAN_ENABLE"},
            {_IRQ_ENABLE, "IRQ_ENABLE"},
            {_MEM_BUF_SEL, "MEM_BUF_SEL"},
            {_MEM_MODES, "MEM_MODES"},
            {_IRQ_FC, "IRQ_FC"},
            {_FRAME, "FRAME"},
            {_FRAME2, "FRAME2"},
            {_TRIGGER, "TRIGGER"},
            {_READ_DATA, "READ_DATA"},
            {_DISCONNECT, "DISCONNECT"},
            {_EXIT, "EXIT"},
            {_TRIG_CONT, "TRIG_CONT"}
        };

        std::string direction_str = kDirectionMap.count(direction) ? kDirectionMap.at(direction) : "UNKNOWN";
        std::string type_str = kTypeMap.count(type) ? kTypeMap.at(type) : "UNKNOWN";
        std::cout << "Command: " << direction_str << " " << type_str << " " << parameter << std::endl;
    }
};

class App {
public:
    App (std::string ip, uint16_t port) {
        m_host_ip = ip;
        m_port = port;

#ifdef _SERIAL_CALLS_ONLY
        Start_Serial_Thread();
#endif

    }

    ~App () {
#ifdef _SERIAL_CALLS_ONLY
        Close_Serial_Thread();
#endif

        m_client.Stop_Consumer_Thread();
    }

    void Connect () {
        m_app_point.Listen(m_host_ip, m_port);
    }


    void Run () {
        Connect ();   

        m_client.Set_Consumer ([this](const _AppPac &pac) {
            m_app_point.Send(pac);
        });
        m_client.Start_Consumer_Thread();

        bool exit_req = false;
        while (!exit_req) {
            auto packets = m_app_point.Receive();

            for (auto& packet : packets) {
                _Command cmd = Construct_Command(packet);

                switch (cmd.type) {
                    case _HEARTBEAT:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            m_client.fpga_HEARTBEAT,
                            m_client.fpga_HEARTBEAT,
                            "heartbeat: set/get heartbeat value",
                            m_app_point
                        );
                    case _ID:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            m_client.fpga_ID,
                            m_client.fpga_ID,
                            "id: set/get client ID",
                            m_app_point
                        );
                    case _RESET:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            [this](uint32_t) { m_client.adc_RESET(); },
                            []() -> std::vector<uint32_t> { return {}; },
                            "reset: reset the ADC (write-only)",
                            m_app_point
                        );
                    case _BURST_LIMIT:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            m_client.adc_BURST_LIMIT,
                            m_client.adc_BURST_LIMIT,
                            "burst_limit: set the ADC burst limit (write-only)",
                            m_app_point
                        );
                    case _START:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            [this](uint32_t) { m_client.adc_START(); },
                            []() -> std::vector<uint32_t> { return {}; },
                            "start: start ADC acquisition (write-only)",
                            m_app_point
                        );
                    case _PERSIST:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            m_client.adc_START_PERSIST,
                            m_client.adc_START_PERSIST,
                            "persist: set/get whether ADC start signal should persist until cleared",
                            m_app_point
                        );
                    case _PERIOD:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            m_client.cd2_PERIOD,
                            m_client.cd2_PERIOD,
                            "period: set/get CD2 pulse period in clock cycles",
                            m_app_point
                        );
                    case _WIDTH:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            m_client.cd2_WIDTH,
                            m_client.cd2_WIDTH,
                            "width: set/get CD2 pulse width in clock cycles",
                            m_app_point
                        );
                    case _PULSES:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            [](uint32_t) { /* read-only */ },
                            m_client.adc_TOTAL_PULSES,
                            "pulses: get total ADC pulses captured (read-only)",
                            m_app_point
                        );
                    case _FRAME_COUNT:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            [](uint32_t) { /* read-only */ },
                            m_client.adc_FRAME_COUNT,
                            "frame_count: get ADC frame count (read-only)",
                            m_app_point
                        );
                    case _WRITES:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            [](uint32_t) { /* read-only */ },
                            m_client.adc_TOTAL_WRITES,
                            "writes: get total ADC writes to memory (read-only)",
                            m_app_point
                        );
                    case _N_AVG:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            m_client.adc_N_AVERAGE,
                            m_client.adc_N_AVERAGE,
                            "n_avg: set/get number of elements to average for ADC mean calculation",
                            m_app_point
                        );
                    case _DELAY:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            m_client.adc_DELAY,
                            m_client.adc_DELAY,
                            "delay: set/get delay in clock cycles before ADC acquisition start",
                            m_app_point
                        );
                    case _MEAN_ENABLE:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            m_client.adc_ENABLE_MEAN,
                            m_client.adc_ENABLE_MEAN,
                            "mean_enable: set/get whether ADC mean calculation is enabled",
                            m_app_point
                        );
                    case _IRQ_ENABLE:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            m_client.irq_ENABLE,
                            m_client.irq_ENABLE,
                            "irq_enable: set/get whether ADC interrupts are enabled",
                            m_app_point
                        );
                    case _MEM_BUF_SEL:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            [](uint32_t) { /* read-only */ },
                            m_client.adc_MEM_BUF_SEL,
                            "mem_buf_sel: get current ADC memory buffer selection (read-only)",
                            m_app_point
                        );
                    case _MEM_MODES:
                        _APP_IMPLT_DIRZ(
                            cmd,
                            m_client.adc_MEM_MODES,
                            m_client.adc_MEM_MODES,
                            "mem_modes: set/get ADC memory modes (0=normal, 1=ping pong)",
                            m_app_point
                        );
                    case _IRQ_FC:
                            _APP_IMPLT_DIRZ(
                                cmd,
                                m_client.adc_FRAME_CLEAR,
                                []() -> std::vector<uint32_t> { return {}; },
                                "irq_fc: get current ADC IRQ frame count (write-only)",
                                m_app_point
                            );
                    case _FRAME:
                            _APP_IMPLT_DIRZ(
                                cmd,
                                [](uint32_t) { /* read-only */ },
                                m_client.adc_FRAME,
                                "frame: get current ADC frame number (read-only)",
                                m_app_point
                            );
                    case _FRAME2:
                            _APP_IMPLT_DIRZ(
                                cmd,
                                [](uint32_t) { /* read-only */ },
                                m_client.adc_FRAME2,
                                "frame2: get current ADC frame number from second register (read-only)",
                                m_app_point
                            );
                    case _TRIGGER:
                        m_client.Trigger();
                        break;
                    case _TRIG_CONT:
                            _APP_IMPLT_DIRZ(
                                cmd,
                                m_client.Continuous_Trigger,
                                m_client.Continuous_Trigger,
                                "trig_cont: set/get whether continuous triggering is enabled",
                                m_app_point
                            );
                    case _READ_DATA:
                        {
                            // Data readback path is not wired yet in Client API.
                            // Return an empty GET response to keep protocol flow valid.
                            AppPacket packet(_AppPacType::GET);
                            packet.Set_Data({});
                            m_app_point.Send(packet);
                        }
                        break;

                    case _DISCONNECT:
                        {
                            AppPacket packet(_AppPacType::SET);
                            packet.Set_Success(true);
                            m_app_point.Send(packet);
                            exit_req = true;
                            std::cout << "Disconnect command received. Shutting down." << std::endl;
                        }
                        break;
                    
                    case _EXIT:
                        exit_req = true;
                        std::cout << "Exit command received. Shutting down." << std::endl;
                        break;

                    default:
                        std::cerr << "Unknown command type: " << cmd.type << std::endl;
                        break;
                }
            }

        }

    }

    _Command Construct_Command (const _AppPac &p) {
        _Command cmd {};

        if (p.type != CMD) {
            throw std::invalid_argument("packet type is not CMD");
        }

        size_t cmd_bytes = std::min<size_t>(p.length * 4u, 96u);
        if (cmd_bytes == 0) {
            // Fallback for malformed/local packets that did not set length.
            cmd_bytes = strnlen(reinterpret_cast<const char*>(p.command), sizeof(p.command));
        }

        std::string cmd_str(reinterpret_cast<const char*>(p.command), cmd_bytes);

        const size_t null_pos = cmd_str.find('\0');
        if (null_pos != std::string::npos) {
            cmd_str.erase(null_pos);
        }

        std::istringstream iss(cmd_str);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) {
            std::transform(token.begin(), token.end(), token.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            throw std::invalid_argument("invalid command format");
        }

        // Single-token control commands.
        if (tokens[0] == "disconnect") {
            cmd.direction = _SET;
            cmd.type = _DISCONNECT;
            cmd.parameter = 0;
            return cmd;
        }
        if (tokens[0] == "exit") {
            cmd.direction = _SET;
            cmd.type = _EXIT;
            cmd.parameter = 0;
            return cmd;
        }

        if (tokens.size() < 2) {
            throw std::invalid_argument("invalid command format");
        }

        if (tokens[0] == "set") {
            cmd.direction = _SET;
        } else if (tokens[0] == "get") {
            cmd.direction = _GET;
        } else if (tokens[0] == "help") {
            cmd.direction = _HELP;
        } else {
            throw std::invalid_argument("unknown command direction: " + tokens[0]);
        }

        static const std::unordered_map<std::string, _CommandType> kTypeMap = {
            {"heartbeat", _HEARTBEAT},
            {"id", _ID},
            {"reset", _RESET},
            {"burst_limit", _BURST_LIMIT},
            {"start", _START},
            {"persist", _PERSIST},
            {"period", _PERIOD},
            {"width", _WIDTH},
            {"pulses", _PULSES},
            {"frame_count", _FRAME_COUNT},
            {"writes", _WRITES},
            {"n_avg", _N_AVG},
            {"delay", _DELAY},
            {"mean_enable", _MEAN_ENABLE},
            {"irq_enable", _IRQ_ENABLE},
            {"mem_buf_sel", _MEM_BUF_SEL},
            {"mem_modes", _MEM_MODES},
            {"irq_fc", _IRQ_FC},
            {"frame", _FRAME},
            {"frame2", _FRAME2},
            {"trigger", _TRIGGER},
            {"read_data", _READ_DATA},
            {"disconnect", _DISCONNECT},
            {"exit", _EXIT},
            {"trig_cont", _TRIG_CONT}
        };

        auto it = kTypeMap.find(tokens[1]);
        if (it == kTypeMap.end()) {
            throw std::invalid_argument("unknown command type: " + tokens[1]);
        }
        cmd.type = it->second;

        cmd.parameter = 0;
        if (tokens.size() >= 3) {
            if (tokens[2] == "true") {
                cmd.parameter = 1;
            } else if (tokens[2] == "false") {
                cmd.parameter = 0;
            } else {
                cmd.parameter = static_cast<uint32_t>(std::stoul(tokens[2]));
            }
        }

        return cmd;
    }

private:
    AppPoint m_app_point;
    std::string m_host_ip = "192.168.2.10";
    uint16_t m_port = 8000;

    void Consumer(_AppPac &pac) {
        // All this does is that it Sends the packet out
        m_app_point.Send(pac);
    }

    Client m_client;

    // Serial-only functions
#ifdef _SERIAL_CALLS_ONLY
    std::thread m_serial_thread;
    std::atomic<bool> m_serial_thread_running {false};
    void Start_Serial_Thread () {
        m_serial_thread_running = true;
        m_serial_thread = std::thread([this]() {
            char c;
            while (m_serial_thread_running.load(std::memory_order_relaxed)) {
                while (read(
                    STDIN_FILENO, &c, 1) == 1
                ) {
                    if (c == '\n') {
                        m_client.Continuous_Trigger(true);
                        return; // Close thread
                    }
                }
            }
        });
    }

    void Close_Serial_Thread () {
        m_serial_thread_running = false;
        if (m_serial_thread.joinable()) {
            m_serial_thread.join();
        }
    }

#endif
};

int main (int argc, char* argv[]) {
    std::string ip = "192.168.2.10";
    if (argc > 1) {
        ip = argv[1];
    }

    App app(ip, 8000);

    app.Run();


}