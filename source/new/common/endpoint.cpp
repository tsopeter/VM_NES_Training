#include "endpoint.hpp"

#include <chrono>
#include <cstring>
#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

EndPoint::EndPoint() {
}

EndPoint::~EndPoint() {
	Stop_IO_Threads();
	Close_Sockets();
}

void EndPoint::Listen(const std::string& ip, uint16_t port) {
	Stop_IO_Threads();
	Close_Sockets();

	m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_server_socket < 0) {
		throw std::runtime_error("failed to create server socket");
	}

	int opt = 1;
	if (setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		Close_Sockets();
		throw std::runtime_error("failed to set SO_REUSEADDR");
	}

	if (setsockopt(m_server_socket, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
		Close_Sockets();
		throw std::runtime_error("failed to set TCP_NODELAY");
	}

	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (ip.empty() || ip == "0.0.0.0") {
		server_addr.sin_addr.s_addr = INADDR_ANY;
	} else if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
		Close_Sockets();
		throw std::runtime_error("invalid listen IP address");
	}

	if (bind(m_server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
		Close_Sockets();
		throw std::runtime_error("failed to bind server socket");
	}

	if (listen(m_server_socket, 1) < 0) {
		Close_Sockets();
		throw std::runtime_error("failed to listen on server socket");
	}

	sockaddr_in client_addr{};
	socklen_t client_addr_len = sizeof(client_addr);
	m_connection_socket = accept(m_server_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
	if (m_connection_socket < 0) {
		Close_Sockets();
		throw std::runtime_error("failed to accept incoming connection");
	}

	if (setsockopt(m_connection_socket, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
		Close_Sockets();
		throw std::runtime_error("failed to set TCP_NODELAY for accepted socket");
	}

	m_connected.store(true, std::memory_order_release);
	Start_IO_Threads();
}

void EndPoint::Connect(const std::string& ip, uint16_t port) {
	Stop_IO_Threads();
	Close_Sockets();

	m_connection_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_connection_socket < 0) {
		throw std::runtime_error("failed to create client socket");
	}

	int opt = 1;
	if (setsockopt(m_connection_socket, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
		Close_Sockets();
		throw std::runtime_error("failed to set TCP_NODELAY");
	}

	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
		Close_Sockets();
		throw std::runtime_error("invalid target IP address");
	}

	if (connect(m_connection_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
		Close_Sockets();
		throw std::runtime_error("failed to connect");
	}

	m_connected.store(true, std::memory_order_release);
	Start_IO_Threads();
}

void EndPoint::Send(const std::vector<uint8_t>& data) {
	if (!m_connected.load(std::memory_order_acquire) || data.empty()) {
		return;
	}
	m_send_queue.enqueue(data);
}

std::vector<uint8_t> EndPoint::Receive() {
	std::vector<uint8_t> data;
	while (m_connected.load(std::memory_order_acquire) ||
		   m_receive_thread_running.load(std::memory_order_acquire)) {
		if (m_receive_queue.try_dequeue(data)) {
			return data;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	if (m_receive_queue.try_dequeue(data)) {
		return data;
	}

	return {};
}

void EndPoint::Start_IO_Threads() {
	m_send_thread_running.store(true, std::memory_order_release);
	m_send_thread = std::thread([this]() {
		std::vector<uint8_t> data;
		while (m_send_thread_running.load(std::memory_order_acquire) &&
			   m_connected.load(std::memory_order_acquire)) {
			if (!m_send_queue.try_dequeue(data)) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			size_t total_sent = 0;
			while (total_sent < data.size() &&
				   m_send_thread_running.load(std::memory_order_acquire) &&
				   m_connected.load(std::memory_order_acquire)) {
				ssize_t sent = send(
					m_connection_socket,
					data.data() + total_sent,
					data.size() - total_sent,
					0
				);

				if (sent < 0) {
					if (errno == EINTR) {
						continue;
					}
					m_connected.store(false, std::memory_order_release);
					break;
				}

				if (sent == 0) {
					m_connected.store(false, std::memory_order_release);
					break;
				}

				total_sent += static_cast<size_t>(sent);
			}
		}
	});

	m_receive_thread_running.store(true, std::memory_order_release);
	m_receive_thread = std::thread([this]() {
		std::vector<uint8_t> buffer(4096);

		while (m_receive_thread_running.load(std::memory_order_acquire) &&
			   m_connected.load(std::memory_order_acquire)) {
			ssize_t received = recv(m_connection_socket, buffer.data(), buffer.size(), 0);

			if (received < 0) {
				if (errno == EINTR) {
					continue;
				}
				m_connected.store(false, std::memory_order_release);
				break;
			}

			if (received == 0) {
				m_connected.store(false, std::memory_order_release);
				break;
			}

			std::vector<uint8_t> data(buffer.begin(), buffer.begin() + received);
			m_receive_queue.enqueue(data);
		}
	});
}

void EndPoint::Stop_IO_Threads() {
	m_send_thread_running.store(false, std::memory_order_release);
	m_receive_thread_running.store(false, std::memory_order_release);
	m_connected.store(false, std::memory_order_release);

	if (m_connection_socket >= 0) {
		shutdown(m_connection_socket, SHUT_RDWR);
	}

	if (m_send_thread.joinable()) {
		m_send_thread.join();
	}

	if (m_receive_thread.joinable()) {
		m_receive_thread.join();
	}
}

void EndPoint::Close_Sockets() {
	if (m_connection_socket >= 0) {
		close(m_connection_socket);
		m_connection_socket = -1;
	}

	if (m_server_socket >= 0) {
		close(m_server_socket);
		m_server_socket = -1;
	}
}

