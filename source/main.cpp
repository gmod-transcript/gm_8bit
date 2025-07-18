#define NO_MALLOC_OVERRIDE

#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/FactoryLoader.hpp>
#include <scanning/symbolfinder.hpp>
#include <detouring/hook.hpp>
#include <iostream>
#include <iclient.h>
#include <cstring>
#include <fstream>
#include "ivoicecodec.h"
#include "net.h"
#include "steam_voice.h"
#include <GarrysMod/Symbol.hpp>
#include <cstdint>
#include "opus_framedecoder.h"

#define STEAM_PCKT_SZ sizeof(uint64_t) + sizeof(CRC32_t)
#ifdef SYSTEM_WINDOWS
	#include <windows.h>

	const std::vector<Symbol> BroadcastVoiceSyms = {
#if defined ARCHITECTURE_X86
		Symbol::FromSignature("\x55\x8B\xEC\xA1****\x83\xEC\x50\x83\x78\x30\x00\x0F\x84****\x53\x8D\x4D\xD8\xC6\x45\xB4\x01\xC7\x45*****"),
		Symbol::FromSignature("\x55\x8B\xEC\x8B\x0D****\x83\xEC\x58\x81\xF9****"),
		Symbol::FromSignature("\x55\x8B\xEC\xA1****\x83\xEC\x50"),
#elif defined ARCHITECTURE_X86_64
		Symbol::FromSignature("\x48\x89\x5C\x24*\x56\x57\x41\x56\x48\x81\xEC****\x8B\xF2\x4C\x8B\xF1"),
#endif
	};
#endif

#ifdef SYSTEM_LINUX
	#include <dlfcn.h>
	const std::vector<Symbol> BroadcastVoiceSyms = {
		Symbol::FromName("_Z21SV_BroadcastVoiceDataP7IClientiPcx"),
		Symbol::FromSignature("\x55\x48\x8D\x05****\x48\x89\xE5\x41\x57\x41\x56\x41\x89\xF6\x41\x55\x49\x89\xFD\x41\x54\x49\x89\xD4\x53\x48\x89\xCB\x48\x81\xEC****\x48\x8B\x3D****\x48\x39\xC7\x74\x25"),
	};
#endif

Net* net_handl = nullptr;
std::string auth_token = "";

// Function to read authentication token from file
bool LoadAuthToken() {
	std::ifstream tokenFile("garrysmod/lua/bin/transcript.mdp");
	if (!tokenFile.is_open()) {
		std::cout << "[TRANSCRIPT] ERROR: Authentication token file 'garrysmod/lua/bin/transcript.mdp' is missing!" << std::endl;
		return false;
	}
	
	std::getline(tokenFile, auth_token);
	tokenFile.close();
	
	// Remove any trailing whitespace/newlines
	auth_token.erase(auth_token.find_last_not_of(" \t\n\r\f\v") + 1);
	
	if (auth_token.empty()) {
		std::cout << "[TRANSCRIPT] ERROR: Authentication token file is empty!" << std::endl;
		return false;
	}
	
	std::cout << "[TRANSCRIPT] Authentication token loaded successfully." << std::endl;
	return true;
}

typedef void (*SV_BroadcastVoiceData)(IClient* cl, int nBytes, char* data, int64 xuid);
Detouring::Hook detour_BroadcastVoiceData;

void hook_BroadcastVoiceData(IClient* cl, uint nBytes, char* data, int64 xuid) {
	// Always broadcast voice data to transcript.linv.dev (only if token is loaded)
	if (!auth_token.empty() && nBytes > sizeof(uint64_t)) {
		// Get the user's steamid64, put it at the beginning of the buffer
#if defined ARCHITECTURE_X86
		uint64_t id64 = *(uint64_t*)((char*)cl + 181);
#else
		uint64_t id64 = *(uint64_t*)((char*)cl + 189);
#endif

		static char broadcastBuffer[20 * 1024];
		*(uint64_t*)broadcastBuffer = id64;

		// Transfer the packet data to our broadcast buffer
		size_t toCopy = nBytes - sizeof(uint64_t);
		memcpy(broadcastBuffer + sizeof(uint64_t), data + sizeof(uint64_t), toCopy);

		// Broadcast to transcript.linv.dev/broadcast with authentication
		net_handl->SendAuthenticatedPacket("transcript.linv.dev", 443, broadcastBuffer, nBytes, auth_token);
	}

	// Pass through the original voice data
	return detour_BroadcastVoiceData.GetTrampoline<SV_BroadcastVoiceData>()(cl, nBytes, data, xuid);
}




GMOD_MODULE_OPEN()
{
	// Load authentication token
	if (!LoadAuthToken()) {
		LUA->ThrowError("Failed to load authentication token from garrysmod/lua/bin/transcript.mdp");
		return 0;
	}

	SourceSDK::ModuleLoader engine_loader("engine");
	SymbolFinder symfinder;

	void* sv_bcast = nullptr;

	for (const auto& sym : BroadcastVoiceSyms) {
		sv_bcast = symfinder.Resolve(engine_loader.GetModule(), sym.name.c_str(), sym.length);

		if (sv_bcast)
			break;
	}

	if (sv_bcast == nullptr) {
		LUA->ThrowError("Could not locate SV_BroadcastVoice symbol!");
	}

	detour_BroadcastVoiceData.Create(Detouring::Hook::Target(sv_bcast), reinterpret_cast<void*>(&hook_BroadcastVoiceData));
	detour_BroadcastVoiceData.Enable();

	net_handl = new Net();

	return 0;
}

GMOD_MODULE_CLOSE()
{
	detour_BroadcastVoiceData.Disable();
	detour_BroadcastVoiceData.Destroy();

	delete net_handl;

	return 0;
}
