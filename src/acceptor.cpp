#include "fz/net/acceptor.h"

#include "fz/net/session.h"

namespace fz::net {

Acceptor::Acceptor(std::shared_ptr<Loop> loop, std::string_view ip,
                   std::uint16_t port)
    : _loop{std::move(loop)},
      _acceptor{_loop->getIoContext()},
      _ip{ip},
      _port{port} {}

auto Acceptor::start() -> void {
  _loop->postTask([this] { listen(); });
}

auto Acceptor::stop() -> void { _acceptor.close(); }

auto Acceptor::setNewSessionCallback(
    std::function<std::shared_ptr<Session>()> new_session_callback) -> void {
  _new_session_callback = std::move(new_session_callback);
}

auto Acceptor::listen() -> void {
  auto endpoint = asio::ip::tcp::endpoint{asio::ip::make_address(_ip), _port};
  _acceptor.open(endpoint.protocol());
  _acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
  _acceptor.bind(endpoint);
  _acceptor.listen();
  accept();
}

static auto handleAccept(const auto& ec, auto& new_session,
                         const auto& acceptor) -> int {
  if (ec) {
    new_session->disconnect();
    return 1;
  }

  new_session->start();

  if (!acceptor.is_open()) {
    return 1;
  }

  return 0;
}

auto Acceptor::accept() -> void {
  auto new_session = _new_session_callback();
  _acceptor.async_accept(new_session->socket(),
                         [this, new_session](const auto& ec) {
                           if (handleAccept(ec, new_session, _acceptor) != 0) {
                             return;
                           }

                           accept();
                         });
}

}  // namespace fz::net
