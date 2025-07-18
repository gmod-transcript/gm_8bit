// net.h
#pragma once
#include <cstdint>
#include <string>
#include <thread>

// si vous compilez avec -DUSE_WEBSOCKET
#ifdef USE_WEBSOCKET
# include <websocketpp/config/asio_no_tls_client.hpp>
# include <websocketpp/client.hpp>
using ws_client = websocketpp::client<websocketpp::config::asio_client>;
#endif

class Net {
public:
  // mode == false → UDP ; mode == true → WebSocket vers uri (ex: "ws://127.0.0.1:8080")
  Net(bool mode = false, const std::string& uri = "");
  ~Net();

  // identique à avant
  void SendPacket(const char* dest_or_empty, uint16_t port_or_unused,
                  const char* buffer, uint32_t len);

private:
  bool                  m_wsMode;
#ifdef USE_WEBSOCKET
  ws_client             m_ws;
  websocketpp::connection_hdl m_hdl;
  std::thread           m_wsThread;
#endif
  int                   m_socket; // votre socket UDP d’origine
};
