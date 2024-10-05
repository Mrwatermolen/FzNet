#include "net/acceptor.h"

#include "net/common/log.h"
#include "net/session.h"

namespace fz::net {

Acceptor::Acceptor(std::shared_ptr<Loop> loop, std::string_view ip,
                   uint16_t port)
    : _loop{std::move(loop)},
      _acceptor{_loop->getIoContext()},
      _ip{ip},
      _port{port} {
  LOG_INFO("Acceptor created.{}", "");
}

auto Acceptor::start() -> void {
  LOG_INFO("Acceptor start.{}", "");
  _loop->postTask([this] { listen(); });
}

auto Acceptor::stop() -> void { _acceptor.close(); }

auto Acceptor::setNewSessionCallback(
    std::function<std::shared_ptr<Session>()> new_session_callback) -> void {
  _new_session_callback = std::move(new_session_callback);
}

auto Acceptor::listen() -> void {
  LOG_INFO("Acceptor listen.{}", "");
  LOG_DEBUG("Acceptor listen on {}:{}", _ip, _port);
  auto endpoint = asio::ip::tcp::endpoint{asio::ip::make_address(_ip), _port};
  _acceptor.open(endpoint.protocol());
  _acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
  _acceptor.bind(endpoint);
  _acceptor.listen();
  accept();
}

auto Acceptor::accept() -> void {
  auto new_session = _new_session_callback();
  _acceptor.async_accept(
      new_session->socket(), [this, new_session](const auto& ec) {
        if (ec) {
          LOG_ERROR("Acceptor error: {}", ec.message());
          new_session->disconnect();
          return;
        }

        LOG_DEBUG("Acceptor accept. Remote: {}:{}",
                  new_session->socket().remote_endpoint().address().to_string(),
                  new_session->socket().remote_endpoint().port());
        new_session->start();

        if (!_acceptor.is_open()) {
          LOG_WARN("Acceptor is not open.{}", "");
          return;
        }

        accept();
      });
}

}  // namespace fz::net
