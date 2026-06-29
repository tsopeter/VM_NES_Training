#include "apppac.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
std::string Packet_Type_Name(_AppPacType type) {
	switch (type) {
		case SET:  return "SET";
		case GET:  return "GET";
		case HELP: return "HELP";
		case DATA: return "DATA";
		case CMD:  return "CMD";
		default:            return "UNKNOWN";
	}
}

void Print_Packets(const std::string& tag, const std::vector<_AppPac>& packets) {
	for (const auto& raw : packets) {
		std::cout << tag
				  << " type=" << Packet_Type_Name(raw.type)
				  << ", length=" << raw.length;

		if (raw.type == SET) {
			std::cout << ", success=" << (raw.success ? "true" : "false");
		} else if (raw.type == GET || raw.type == DATA) {
			std::cout << ", values=";
			for (uint32_t i = 0; i < raw.length && i < 24; ++i) {
				if (i > 0) {
					std::cout << ',';
				}
				std::cout << raw.data[i];
			}
		} else if (raw.type == HELP) {
			size_t help_bytes = std::min<size_t>(raw.length * 4u, 96u);
			std::string text(reinterpret_cast<const char*>(raw.help_message), help_bytes);
			std::cout << ", help=\"" << text << "\"";
		} else if (raw.type == CMD) {
			size_t cmd_bytes = std::min<size_t>(raw.length * 4u, 96u);
			std::string text(reinterpret_cast<const char*>(raw.command), cmd_bytes);
			std::cout << ", cmd=\"" << text << "\"";
		}

		std::cout << '\n';
	}
}
}  // namespace

int main() {
	const std::string ip = "127.0.0.1";
	const uint16_t port = 8000;

	AppPoint host;
	AppPoint client;

	std::thread host_thread([&host, &ip, port]() {
		try {
			std::cout << "[host] listening on " << ip << ':' << port << '\n';
			host.Listen(ip, port);
			std::cout << "[host] client connected\n";

			auto incoming = host.Receive();
			Print_Packets("[host] received:", incoming);

			AppPacket data_reply(DATA);
			data_reply.Set_Data({111, 222, 333});
			host.Send(data_reply);
			std::cout << "[host] sent DATA reply\n";
		} catch (const std::exception& e) {
			std::cerr << "[host] error: " << e.what() << '\n';
		}
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	try {
		std::cout << "[client] connecting to " << ip << ':' << port << '\n';
		client.Connect(ip, port);
		std::cout << "[client] connected\n";

		AppPacket request(CMD);
		request.Set_Command("get adc_frame_count");
		client.Send(request);
		std::cout << "[client] sent CMD packet\n";

		auto response = client.Receive();
		Print_Packets("[client] received:", response);
	} catch (const std::exception& e) {
		std::cerr << "[client] error: " << e.what() << '\n';
		if (host_thread.joinable()) {
			host_thread.join();
		}
		return 1;
	}

	if (host_thread.joinable()) {
		host_thread.join();
	}

	std::cout << "[example] done\n";
	return 0;
}
