#ifndef __FZ_NET_SESSION_H__
#define __FZ_NET_SESSION_H__

#include <asio.hpp>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>

#include "fz/net/common/buffer.h"
#include "fz/net/loop.h"

namespace fz::net {

class Session : public std::enable_shared_from_this<Session> {
 public:
  constexpr static auto DEFAULT_RECONNECT_TIMES = 3;

  constexpr static auto DEFAULT_RECONNECT_DELAY_MS = 500;

 public:
  Session(const Session&) = delete;

  Session(Session&&) noexcept = delete;

  auto operator=(const Session&) -> Session& = delete;

  auto operator=(Session&&) noexcept -> Session& = delete;

  explicit Session(std::shared_ptr<Loop> loop)
      : _loop{std::move(loop)},
        _socket{_loop->getIoContext()},
        _timer{_loop->getIoContext()},
        _id{reinterpret_cast<std::uint64_t>(this)} {}

  virtual ~Session() = default;

  auto socket() -> auto& { return _socket; }

  auto socket() const -> auto& { return _socket; }

  auto start() -> void;

  auto disconnect() -> void;

  auto send(const Buffer& buffer) -> void;

  auto setConnectCallback(
      std::function<void(std::shared_ptr<Session>)> callback) {
    _connect_callback = std::move(callback);
  }

  auto setReadCallback(
      std::function<void(std::shared_ptr<Session>, Buffer&)> callback) {
    _read_callback = std::move(callback);
  }

  auto setDisconnectCallback(
      std::function<void(std::shared_ptr<Session>)> callback) {
    _disconnect_callback = std::move(callback);
  }

  // Following functions are unsafe in multi-threading environment
  auto reconnect() const { return _reconnect; }

  auto setReconnect(bool reconnect) { _reconnect = reconnect; }

  auto reconnectTimes() const { return _reconnect_times; }

  auto setReconnectTimes(int reconnect_times) {
    _reconnect_times = reconnect_times;
  }

  auto reconnectDelay() const { return _reconnect_delay_ms; }

  auto setReconnectDelay(std::size_t reconnect_delay_ms) {
    _reconnect_delay_ms = reconnect_delay_ms;
  }

  auto connect(const std::string& ip, std::uint16_t port, bool reconnect)
      -> void;

  auto reconnect(const std::string& ip, std::uint16_t port) -> void;

  // Debug info
  auto id() const { return _id; }

  auto remoteIp() const { return _remote_ip; }

  auto remotePort() const { return _remote_port; }

 private:
  auto read() -> void;

  auto write() -> void;

 private:
  std::shared_ptr<Loop> _loop;
  std::queue<Buffer> _unsent_buffers;
  std::mutex _mutex;  // for queue
  Buffer _write_buffer;
  Buffer _read_buffer;
  asio::ip::tcp::socket _socket;
  std::function<void(std::shared_ptr<Session>)> _connect_callback;
  std::function<void(std::shared_ptr<Session>, Buffer&)> _read_callback;
  std::function<void(std::shared_ptr<Session>)> _disconnect_callback;
  bool _reconnect{false};
  int _reconnect_times{DEFAULT_RECONNECT_TIMES};
  std::size_t _reconnect_delay_ms{DEFAULT_RECONNECT_DELAY_MS};
  asio::steady_timer _timer;

  // Debug info
  std::uint64_t _id;
  std::string _remote_ip;
  std::uint16_t _remote_port{};
};

}  // namespace fz::net

#endif  // __FZ_NET_SESSION_H__
