#ifndef __FZ_NET_ACCEPTOR_H__
#define __FZ_NET_ACCEPTOR_H__

#include <functional>
#include <memory>
#include <string_view>
#include <cstdint>

#include "fz/net/loop.h"

namespace fz::net {

class Session;

class Acceptor {
 public:
  Acceptor(std::shared_ptr<Loop> loop, std::string_view ip, std::uint16_t port);

  [[nodiscard]] auto ip() const  { return _ip; }

  [[nodiscard]] auto port() const { return _port; }

  auto start() -> void;

  auto stop() -> void;

  auto setNewSessionCallback(
      std::function<std::shared_ptr<Session>()> new_session_callback) -> void;

 private:
  auto listen() -> void;

  auto accept() -> void;

 private:
  std::shared_ptr<Loop> _loop;
  asio::ip::tcp::acceptor _acceptor;
  std::string _ip;
  std::uint16_t _port;
  std::function<std::shared_ptr<Session>()> _new_session_callback;
};

}  // namespace fz::net

#endif  // __FZ_NET_ACCEPTOR_H__
