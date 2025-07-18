// net.cpp
#include "net.h"
#ifdef USE_WEBSOCKET
# include <websocketpp/common/thread.hpp>
# include <websocketpp/common/memory.hpp>
#endif

#ifdef _WIN32
# include <winsock2.h>
# include <WS2tcpip.h>
#endif
#ifdef __linux
# include <arpa/inet.h>
# include <sys/socket.h>
# include <unistd.h>
#endif

Net::Net(bool mode, const std::string& uri)
  : m_wsMode(mode)
{
  if (m_wsMode) {
#ifdef USE_WEBSOCKET
    // initialisation de l’ASIO loop
    m_ws.init_asio();

    // on récupère le handle à l’ouverture
    m_ws.set_open_handler([this](websocketpp::connection_hdl h){
      m_hdl = h;
    });

    // connexion
    websocketpp::lib::error_code ec;
    auto conn = m_ws.get_connection(uri, ec);
    if (ec) {
      throw std::runtime_error("WebSocket get_connection failed: " + ec.message());
    }
    m_ws.connect(conn);

    // on lance l’IO loop dans un thread séparé
    m_wsThread = std::thread([this](){
      m_ws.run();
    });
#else
    throw std::runtime_error("Compiler avec -DUSE_WEBSOCKET pour activer le mode WS");
#endif
  } else {
    // ----- votre init UDP existant -----
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
      throw "WSAStartup failed";
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) throw "UDP socket failed";
#else
    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket < 0) throw "UDP socket failed";
#endif
  }
}

void Net::SendPacket(const char* dest, uint16_t port,
                     const char* buffer, uint32_t len)
{
  if (m_wsMode) {
#ifdef USE_WEBSOCKET
    // envoi en binaire
    m_ws.send(m_hdl, buffer, len, websocketpp::frame::opcode::binary);
#endif
  } else {
    // ----- votre sendto UDP existant -----
    sockaddr_in dest_str{};
    dest_str.sin_family = AF_INET;
    dest_str.sin_port   = htons(port);
    inet_pton(AF_INET, dest, &dest_str.sin_addr);
    sendto(m_socket, buffer, len, 0,
           (sockaddr*)&dest_str, sizeof(dest_str));
  }
}

Net::~Net() {
  if (m_wsMode) {
#ifdef USE_WEBSOCKET
    m_ws.stop();
    if (m_wsThread.joinable())
      m_wsThread.join();
#endif
  } else {
#ifdef _WIN32
    closesocket(m_socket);
    WSACleanup();
#else
    close(m_socket);
#endif
  }
}
