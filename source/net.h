#pragma once
#include <cstdint>
#include <string>

class Net {
public:
	Net();
	~Net();
	void SendPacket(const char* dest, uint16_t port, const char* buffer, uint32_t len);
	void SendAuthenticatedPacket(const char* dest, uint16_t port, const char* buffer, uint32_t len, const std::string& token);

private:
	int m_socket;
};