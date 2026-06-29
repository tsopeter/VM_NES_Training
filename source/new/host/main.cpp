#include "host.hpp"
#include "common/apppac.hpp"

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include "common/third-party/concurrentqueue.h"

namespace {

moodycamel::ConcurrentQueue<int> frame_counts;
moodycamel::ConcurrentQueue<std::vector<int>> full_values;
moodycamel::ConcurrentQueue<int> values;
volatile std::sig_atomic_t g_stop_requested = 0;

void Handle_SigInt(int) {
    g_stop_requested = 1;
}

void Save_Frame_Counts_Csv(const std::string& path) {
    std::vector<int> m_values;
    std::vector<int> m_frame_counts;
    std::vector<std::vector<int>> m_full_values;
    int value = 0;
    int frame_count = 0;
    while (values.try_dequeue(value)) {
        m_values.push_back(value);
    }
    while (frame_counts.try_dequeue(frame_count)) {
        m_frame_counts.push_back(frame_count);
    }
    std::vector<int> full_value;
    while (full_values.try_dequeue(full_value)) {
        m_full_values.push_back(full_value);
    }

    std::ofstream out(path);
    if (!out) {
        std::cerr << "Failed to open " << path << " for writing" << std::endl;
        return;
    }

    out << "frame_count,value\n";
    for (size_t i = 0; i < m_frame_counts.size(); ++i) {
        out << m_frame_counts[i] << "," << m_values[i] << "\n";
    }

    std::cout << "Saved " << m_frame_counts.size() << " frame_counts to " << path << std::endl;

    std::ofstream full_out("full_" + path);
    if (!full_out) {
        std::cerr << "Failed to open " << "full_" + path << " for writing" << std::endl;
        return;
    }

    // Return as CSV with header "frame_count,value_0,value_1,...,value_23"
    full_out << "frame_count";
    for (int i = 0; i < 24; ++i) {
        full_out << ",value_" << i;
    }
    full_out << "\n";

    for (size_t i = 0; i < m_full_values.size(); ++i) {
        full_out << m_frame_counts[i];
        for (int j = 0; j < 24; ++j) {
            full_out << "," << m_full_values[i][j];
        }
        full_out << "\n";
    }

    std::cout << "Saved " << m_full_values.size() << " full frame values to " << "full_" + path << std::endl;
}


void Print_Results(const std::vector<_AppPac>& packets) {
    size_t result_index = 0;

    for (const auto& packet : packets) {
        switch (packet.type) {
            case SET:
                std::cout << "result_" << result_index++ << ": "
                          << (packet.success ? "success" : "failure") << "\n";
                break;

            case GET: {
                std::cout << "result_" << result_index++ << ": " << 
                packet.data[0] << "\n";
                break;
            }
            case DATA: {
                size_t count = std::min<size_t>(packet.length, 24);
                std::cout << "result_" << result_index++ << ": [";
                
                uint32_t min_value = UINT32_MAX;
                uint32_t min_index = 0;
                std::vector<int> values_vec;
                for (size_t i = 0; i < count; ++i) {
                    if (packet.data[i] < min_value) {
                        min_value = packet.data[i];
                        min_index = i;
                    }
                    values_vec.push_back(packet.data[i]);
                    std::cout<<packet.data[i] << ' ';
                }
                
                if (count == 24) {
                    frame_counts.enqueue(packet.frame_id);
                    values.enqueue(min_index);
                    // full_values.enqueue(values_vec);
                }
                std::cout << "]" << "frame_id=" << packet.frame_id << "\n";
                break;
            }

            case HELP: {
                size_t bytes = std::min<size_t>(packet.length * 4u, 96u);
                std::string text(reinterpret_cast<const char*>(packet.help_message), bytes);
                size_t nul = text.find('\0');
                if (nul != std::string::npos) {
                    text.erase(nul);
                }
                std::cout << "result_" << result_index++ << ": " << text << "\n";
                break;
            }

            case CMD: {
                size_t bytes = std::min<size_t>(packet.length * 4u, 96u);
                std::string text(reinterpret_cast<const char*>(packet.command), bytes);
                size_t nul = text.find('\0');
                if (nul != std::string::npos) {
                    text.erase(nul);
                }
                std::cout << "result_" << result_index++ << ": " << text << std::endl;
                break;
            }

            default:
                std::cout << "result_" << result_index++ << ": (unknown packet type)" << std::endl;
                break;
        }
    }

    if (result_index == 0) {
        std::cout << "result_0: (no response)" << std::endl;
    }
}

void Run_Interval_Command (Host &host, int count, double period_ms) {
    using clock = std::chrono::high_resolution_clock;

    auto period = std::chrono::duration<double, std::milli>(period_ms);
    auto next_time = clock::now();

    for (int k = 0; k < count; ++k) {
        host.Send_Command("get trigger");
        next_time += std::chrono::duration_cast<clock::duration>(period);
        std::this_thread::sleep_until(next_time);
    }
}

bool Handle_Trigger_String (Host &host, const std::string &command) {
    std::istringstream iss(command);

    std::string cmd;
    iss >> cmd;

    if (cmd == "i") {
        int count = 0; 
        double period_ms = 0.0;

        if (!(iss >> count >> period_ms)) {
            std::cerr << "Usage: i <count> <period_ms>\n";
            return false;
        }

        if (count <= 0 || period_ms <= 0.0) {
            std::cerr << "Error: count and period_ms must be positive\n";
            return false;
        }

        Run_Interval_Command(
            host, count, period_ms
        );
        return true;


    }
    return false;
}
}  // namespace


int main (int argc, char* argv[]) {
    std::string ip = "192.168.2.10";

    std::signal(SIGINT, Handle_SigInt);

    if (argc > 1) {
        ip = argv[1];
    }

    Host host(ip, 8000);

    std::atomic<bool> recv_thread_running{true};
    std::thread recv_thread([&recv_thread_running, &host]() {
        while (recv_thread_running.load(std::memory_order_acquire)) {
            auto responses = host.Receive();
            Print_Results(responses);
        }
    });


    std::string cmd;
    while (true) {
        if (g_stop_requested) {
            std::cout << "\nCtrl-C received. Exiting..." << std::endl;
            break;
        }

        std::cout << ">>> ";
        if (!std::getline(std::cin, cmd)) {
            if (g_stop_requested) {
                std::cout << "\nCtrl-C received. Exiting..." << std::endl;
            }
            break;
        }

        if (cmd.empty()) {
            continue;
        }


        // If the command is
        // we send 
        // "i 100 16.66"
        // where 100 is the number of frames
        // to capture and 16.66 is the interval in ms
        if (Handle_Trigger_String(host, cmd)) {
            continue;
        }

        host.Send_Command(cmd);
        //auto response = host.Receive();
        //Print_Results(response);
        std::cout << "===================" << std::endl;

        if (cmd == "exit" || cmd == "disconnect") {
            break;
        }
    }

    // Ask peer to disconnect so Receive() thread can unwind promptly.
    try {
        host.Send_Command("disconnect");
    } catch (...) {
        // Ignore shutdown-time send failures.
    }

    recv_thread_running.store(false, std::memory_order_release);
    if (recv_thread.joinable()) {
        recv_thread.join();
    }

    Save_Frame_Counts_Csv("fc_0.csv");
}