#ifndef __FZ_NET_SESSION_H__
#define __FZ_NET_SESSION_H__

#include <asio.hpp>
#include <functional>
#include <memory>
#include <queue>
#include <utility>

#include "net/common/buffer.h"
#include "net/loop.h"

namespace fz::net {

class Session : public std::enable_shared_from_this<Session> {
 public:
  explicit Session(std::shared_ptr<Loop> loop)
      : _loop{std::move(loop)}, _socket{_loop->getIoContext()} {}

  virtual ~Session() = default;

  auto& socket() { return _socket; }

  auto& socket() const { return _socket; }

  auto start() -> void;

  auto close() -> void;

  auto send(const Buffer& buffer) -> void;

  auto setReadCallback(
      std::function<void(std::shared_ptr<Session>, Buffer&)> callback) {
    _read_callback = std::move(callback);
  }

 private:
  auto read() -> void;

  auto write() -> void;

 private:
  std::shared_ptr<Loop> _loop;
  std::queue<Buffer> _unsent_buffers;
  Buffer _write_buffer;
  Buffer _read_buffer;
  asio::ip::tcp::socket _socket;
  std::function<void(std::shared_ptr<Session>, Buffer&)> _read_callback;

  // Debug info
  std::uint64_t _id;
  std::string _remote_ip;
  std::uint16_t _remote_port;
};

}  // namespace fz::net

#endif  // __FZ_NET_SESSION_H__
