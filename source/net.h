// net.h
#pragma once

#include <cstdint>
#include <vector>

class Net {
public:
    Net();
    ~Net();

    // dest : adresse IPv4 sous forme de chaîne, port : port UDP,
    // buffer/len : données brutes à chiffrer et envoyer
    void SendPacket(const char* dest, uint16_t port, const char* buffer, uint32_t len);

private:
    int m_socket;
    std::vector<unsigned char> m_pubkey;  // clé publique pour crypto_box_seal
};
