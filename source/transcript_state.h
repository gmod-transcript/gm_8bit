#pragma once
#include <string>
#include <unordered_map>

struct transcriptState {
	int crushFactor = 350;
	float gainFactor = 1.2;
	bool broadcastPackets = false;
	int desampleRate = 2;
	bool useWebSocket = false;
	uint16_t port = 4000;
	std::string ip = "127.0.0.1";
	std::unordered_map<int, std::tuple<IVoiceCodec*, int>> afflictedPlayers;
};
