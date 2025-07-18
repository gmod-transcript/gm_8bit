#pragma once
#include <string>

struct transcriptState {
	bool broadcastPackets = false;
	uint16_t port = 4000;
	std::string ip = "127.0.0.1";
};
