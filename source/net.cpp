// net.cpp
#include "net.h"
#include <sodium.h>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
  #include <winsock2.h>
  #include <WS2tcpip.h>
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
#endif

Net::Net() {
    // 1) Initialisation de libsodium
    if (sodium_init() < 0) {
        throw std::runtime_error("Erreur init libsodium");
    }

    // 2) Embarque la clé publique (32 octets = 64 hex chars)
    const std::string hex =
        "b5aab74e737030a3aa8e9471a41a8260423fb60e2626655526ffb46133ffef05";
    if (hex.size() != crypto_box_PUBLICKEYBYTES * 2) {
        throw std::runtime_error("Public key hex length invalide");
    }
    m_pubkey.resize(crypto_box_PUBLICKEYBYTES);
    for (size_t i = 0; i < m_pubkey.size(); ++i) {
        m_pubkey[i] = static_cast<unsigned char>(
            std::stoul(hex.substr(2*i, 2), nullptr, 16)
        );
    }

    // 3) Création du socket UDP
  #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Invalid UDP socket");
    }
  #else
    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket < 0) {
        throw std::runtime_error("Invalid UDP socket");
    }
  #endif
}

void Net::SendPacket(const char* dest, uint16_t port, const char* buffer, uint32_t len) {
    // Prépare l'adresse destinataire
    sockaddr_in dest_str{};
    dest_str.sin_family = AF_INET;
    dest_str.sin_port   = htons(port);
    inet_pton(AF_INET, dest, &dest_str.sin_addr);

    // Chiffrement AEAD asymétrique (crypto_box_seal)
    unsigned long long cipher_len = len + crypto_box_SEALBYTES;
    std::vector<unsigned char> cipher(cipher_len);

    if (crypto_box_seal(
            cipher.data(),
            reinterpret_cast<const unsigned char*>(buffer),
            len,
            m_pubkey.data()
        ) != 0)
    {
        throw std::runtime_error("crypto_box_seal failed");
    }

    // Envoi du buffer chiffré
    sendto(
        m_socket,
        reinterpret_cast<const char*>(cipher.data()),
        static_cast<int>(cipher_len),
        0,
        reinterpret_cast<sockaddr*>(&dest_str),
        sizeof(dest_str)
    );
}

Net::~Net() {
  #ifdef _WIN32
    closesocket(m_socket);
    WSACleanup();
  #else
    close(m_socket);
  #endif
}
