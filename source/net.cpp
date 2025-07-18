#include "net.h"
#include <cstdint>
#include <string>
#include <sstream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#endif

#ifdef __linux
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#endif

Net::Net() {
#ifdef _WIN32
	WSADATA wsaData;
	int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != NO_ERROR)
		throw "Initialization Error!";

	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_socket == INVALID_SOCKET)
		throw "Invalid socket!";
#elif __linux
	m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_socket < 0)
		throw "Invalid socket!";
#endif
}

void Net::SendPacket(const char* dest, uint16_t port, const char* buffer, uint32_t len) {
	sockaddr_in dest_str;
	dest_str.sin_family = AF_INET;
	dest_str.sin_port = htons(port);
	inet_pton(AF_INET, dest, &dest_str.sin_addr);
	sendto(m_socket, buffer, len, 0, (sockaddr*)&dest_str, sizeof(dest_str));
}

void Net::SendAuthenticatedPacket(const char* dest, uint16_t port, const char* buffer, uint32_t len, const std::string& token) {
	// Create TCP socket for HTTP request
	int tcp_socket;
#ifdef _WIN32
	tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (tcp_socket == INVALID_SOCKET)
		return;
#elif __linux
	tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_socket < 0)
		return;
#endif

	// Resolve hostname
	struct hostent* he = gethostbyname(dest);
	if (!he) {
#ifdef _WIN32
		closesocket(tcp_socket);
#elif __linux
		close(tcp_socket);
#endif
		return;
	}

	// Connect to server
	sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	memcpy(&dest_addr.sin_addr, he->h_addr_list[0], he->h_length);

	if (connect(tcp_socket, (sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
#ifdef _WIN32
		closesocket(tcp_socket);
#elif __linux
		close(tcp_socket);
#endif
		return;
	}

	// Create HTTP POST request
	std::ostringstream request;
	request << "POST /broadcast HTTP/1.1\r\n";
	request << "Host: " << dest << "\r\n";
	request << "Authorization: Bearer " << token << "\r\n";
	request << "Content-Type: application/octet-stream\r\n";
	request << "Content-Length: " << len << "\r\n";
	request << "Connection: close\r\n";
	request << "\r\n";

	std::string header = request.str();
	
	// Send HTTP header
	send(tcp_socket, header.c_str(), header.length(), 0);
	
	// Send voice data
	send(tcp_socket, buffer, len, 0);

	// Close socket
#ifdef _WIN32
	closesocket(tcp_socket);
#elif __linux
	close(tcp_socket);
#endif
}

Net::~Net() {
#ifdef _WIN32
	closesocket(m_socket);
	WSACleanup();
#elif __linux
	close(m_socket);
#endif
}