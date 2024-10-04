#include "net/session.h"

#include <asio.hpp>
#include <cstddef>

#include "net/common/buffer.h"
#include "net/common/log.h"

namespace fz::net {

auto Session::start() -> void {
  // Can't post read to loop here, because the lifetime of the session needs to
  // be.
  _id = reinterpret_cast<std::uint64_t>(this);
  _remote_ip = _socket.remote_endpoint().address().to_string();
  _remote_port = _socket.remote_endpoint().port();
  LOG_DEBUG("Session ID: {}. Remote: {}:{}. Start", _id, _remote_ip,
            _remote_port);
  read();
}

auto Session::close() -> void {
  auto self = shared_from_this();
  LOG_DEBUG("Session close.", "");
  _socket.close();
}

auto Session::send(const Buffer& buffer) -> void {
  auto self = shared_from_this();
  LOG_DEBUG("Session ID: {}. Remote: {}:{}. Send {} bytes.", _id, _remote_ip,
            _remote_port, buffer.readableBytes());
  _unsent_buffers.push(buffer);
  auto still_writing = !_write_buffer.empty();
  if (still_writing) {
    return;
  }

  _write_buffer.resize(
      Buffer::DEFAULT_SIZE);  // avoid buffer from bigging too much
  _loop->postTask([this] { write(); });
}

auto Session::read() -> void {
  auto self = shared_from_this();
  if (_read_buffer.empty()) {
    _read_buffer.resize(Buffer::DEFAULT_SIZE);
  }
  LOG_DEBUG("Session ID: {}. Remote: {}:{}. Read.", _id, _remote_ip,
            _remote_port);

  socket().async_read_some(
      asio::buffer(_read_buffer.writeBegin(), _read_buffer.writeableBytes()),
      [self, this](const auto& ec, auto len) {
        if (ec) {
          LOG_ERROR("Session error: {}", ec.message());
          close();
          return;
        }

        _read_buffer.hasWritten(len);
        LOG_DEBUG("Session ID: {}. Remote: {}:{} Read {} bytes.", _id,
                  _remote_ip, _remote_port, len);
        if (_read_buffer.writeableBytes() <= 0.25 * _read_buffer.capacity()) {
          _read_buffer.resize(_read_buffer.capacity() * 2);
        }
        if (_read_callback) {
          _read_callback(shared_from_this(), _read_buffer);
        }

        read();
      });
}

auto Session::write() -> void {
  auto self = shared_from_this();
  LOG_DEBUG("Session ID: {}. Remote: {}:{}. Write.", _id, _remote_ip,
            _remote_port);
  while (!_unsent_buffers.empty()) {
    auto& buffer = _unsent_buffers.front();
    _write_buffer.append(buffer.readBegin(), buffer.readableBytes());
    _unsent_buffers.pop();
  }

  socket().async_write_some(
      asio::buffer(_write_buffer.readBegin(), _write_buffer.readableBytes()),
      [self, this](const auto& ec, auto len) {
        if (ec) {
          LOG_ERROR("Session error: {}", ec.message());
          close();
          return;
        }

        _write_buffer.retrieve(len);
        LOG_DEBUG("Session ID: {}. Remote: {}:{} Write {} bytes.", _id,
                  _remote_ip, _remote_port, len);
        if (_write_buffer.empty()) {
          LOG_DEBUG("Session ID: {}. Remote: {}:{} Write done.", _id,
                    _remote_ip, _remote_port);
          return;
        }

        write();
      });
}

}  // namespace fz::net
