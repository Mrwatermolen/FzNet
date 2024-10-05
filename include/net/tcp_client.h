#ifndef __FZ_NET_TCP_CLIENT_H__
#define __FZ_NET_TCP_CLIENT_H__

#include <memory>
#include <string>
#include <string_view>

#include "net/session.h"

namespace fz::net {

/**
 * @brief Still in development
 *
 */
class TcpClient {
 public:
  explicit TcpClient(std::shared_ptr<Loop> loop, std::string_view ip,
                     std::uint16_t port)
      : _loop{loop},
        _ip{ip},
        _port{port},
        _session{std::make_shared<Session>(loop)} {}

  auto session() const { return _session; }

  auto session() { return _session; }

  auto connect(bool reconnect = true) -> void {
    _session->connect(_ip, _port, reconnect);
  }

  auto send(const Buffer& buffer) -> void { _session->send(buffer); }

  auto disconnect() -> void { _session->disconnect(); }

  auto setConnectCallback(
      std::function<void(std::shared_ptr<Session>)> callback) -> void {
    _session->setConnectCallback(std::move(callback));
  }

  auto setReadCallback(
      std::function<void(std::shared_ptr<Session>, Buffer&)> callback) -> void {
    _session->setReadCallback(std::move(callback));
  }

  auto setDisconnectCallback(
      std::function<void(std::shared_ptr<Session>)> callback) -> void {
    _session->setDisconnectCallback(std::move(callback));
  }

  auto run() -> void { _loop->start(); }

  auto stop() -> void { _loop->stop(); }

 private:
  std::shared_ptr<Loop> _loop;
  std::string _ip;
  std::uint16_t _port;
  std::shared_ptr<Session> _session;
};

}  // namespace fz::net

#endif  // __FZ_NET_TCP_CLIENT_H__
