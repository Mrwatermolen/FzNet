#ifndef __FZ_NET_TCP_SERVER_H__
#define __FZ_NET_TCP_SERVER_H__

#include <cstddef>
#include <memory>
#include <string_view>

#include "net/acceptor.h"
#include "net/common/buffer.h"
#include "net/loop_pool.h"

namespace fz::net {

class Session;

class TcpServer {
 public:
  TcpServer(std::size_t loop_pool_size, std::string_view ip, uint16_t port);

  auto start() -> void;

  auto stop() -> void;

  template <typename T>
  auto setNewSessionCallback() -> void {
    _acceptor.setNewSessionCallback([this] { return newSession<T>(); });
  }

  auto setReadCallback(
      std::function<void(std::shared_ptr<Session>, Buffer&)> callback) -> void {
    _read_callback = std::move(callback);
  }

 private:
  template <typename T>
    requires std::is_base_of_v<Session, T>
  auto newSession() -> std::shared_ptr<Session> {
    auto session = std::make_shared<T>(_loop_pool->findNext());
    session->setReadCallback(_read_callback);
    return session;
  }

 private:
  std::shared_ptr<LoopPool> _loop_pool;
  Acceptor _acceptor;
  std::function<void(std::shared_ptr<Session>, Buffer&)> _read_callback;
};

}  // namespace fz::net

#endif  // __FZ_NET_TCP_SERVER_H__
