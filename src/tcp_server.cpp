#include "fz/net/tcp_server.h"

#include "fz/net/loop_pool.h"

namespace fz::net {

TcpServer::TcpServer(std::size_t loop_pool_size, std::string_view ip,
                     uint16_t port)
    : _loop_pool{std::make_shared<LoopPool>(loop_pool_size)},
      _acceptor{_loop_pool->findNext(), ip, port} {}

auto TcpServer::start() -> void {
  _loop_pool->start();
  _acceptor.start();
}

auto TcpServer::stop() -> void {
  _acceptor.stop();
  _loop_pool->stop();
}

}  // namespace fz::net
