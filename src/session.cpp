#include "fz/net/session.h"

#include <asio.hpp>
#include <cstddef>
#include <mutex>

#include "fz/net/common/buffer.h"
#include "fz/net/common/log.h"

namespace fz::net {

auto Session::start() -> void {
  if (!socket().is_open()) {
    LOG_ERROR("Socket is not open.");
    return;
  }

  // race condition?
  _remote_ip = _socket.remote_endpoint().address().to_string();
  _remote_port = _socket.remote_endpoint().port();
  LOG_DEBUG("Session ID: {}. Remote: {}:{}. Start", _id, _remote_ip,
            _remote_port);
  if (_connect_callback) {
    _connect_callback(shared_from_this());
  }

  auto self = shared_from_this();
  read();
}

auto Session::disconnect() -> void {
  auto self = shared_from_this();
  if (_disconnect_callback) {
    _disconnect_callback(shared_from_this());
  }

  _loop->postTask([this, self] {
    if (_socket.is_open()) {
      _socket.close();
    }
  });
}

auto Session::connect(const std::string& ip, std::uint16_t port, bool reconnect)
    -> void {
  LOG_DEBUG("Session ID: {}. Remote: {}:{}. Connect.", _id, ip, port);

  auto ep = asio::ip::tcp::endpoint(asio::ip::make_address(ip), port);
  auto self = shared_from_this();

  socket().async_connect(ep, [this, self, ip, port, reconnect](const auto& ec) {
    _remote_ip = ip;
    _remote_port = port;
    _reconnect = reconnect;
    if (ec) {
      if (!_reconnect || _reconnect_times <= 0) {
        this->disconnect();
        return;
      }

      _reconnect_times--;
      _socket.close();
      this->reconnect(_remote_ip, _remote_port);
      return;
    }
    start();
  });
}

static auto handleReconnectError(const auto& ec, const auto& session) -> int {
  if (ec) {
    LOG_ERROR(
        "Session ID: {}. Reconnect error: {}. Remain reconnect times: {}.",
        session->id(), ec.message(), session->reconnectTimes());
    return 1;
  }

  return 0;
}

auto Session::reconnect(const std::string& ip, std::uint16_t port) -> void {
  LOG_DEBUG("Session ID: {}. Remote: {}:{}. Reconnect pending.", _id, ip, port);

  auto self = shared_from_this();
  _timer.expires_after(asio::chrono::milliseconds(_reconnect_delay_ms));
  _timer.async_wait([this, self, ip, port](const auto& ec) {
    if (handleReconnectError(ec, self) != 0) {
      return;
    }

    connect(ip, port, _reconnect);
  });
}

auto Session::send(const Buffer& buffer) -> void {
  auto self = shared_from_this();
  LOG_TRACE("Session ID: {}. Remote: {}:{}. Send {} bytes.", _id, _remote_ip,
            _remote_port, buffer.readableBytes());

  {
    std::scoped_lock lock(_mutex);
    _unsent_buffers.push(buffer);
  }

  _loop->postTask([this] { write(); });
}

static auto handleReadError(const auto& ec, auto id) -> int {
  if (ec) {
    if (ec == asio::error::eof) {
      LOG_DEBUG("Session ID: {}. EOF.", id);
      return 1;
    }

    LOG_ERROR("Session ID: {}. Read error: {}.", id, ec.message());

    return 1;
  }

  return 0;
}

auto Session::read() -> void {
  auto self = shared_from_this();
  if (_read_buffer.empty()) {
    _read_buffer.resize(Buffer::DEFAULT_SIZE);
  }

  socket().async_read_some(
      asio::buffer(_read_buffer.writeBegin(), _read_buffer.writeableBytes()),
      [self, this](const auto& ec, auto len) {
        if (handleReadError(ec, _id) != 0) {
          if (_reconnect && ec != asio::error::eof) {
            reconnect(_remote_ip, _remote_port);
          } else {
            disconnect();
          }

          return;
        }

        _read_buffer.hasWritten(len);
        if (_read_buffer.writeableBytes() <= (_read_buffer.capacity() / 4)) {
          _read_buffer.resize(_read_buffer.capacity() * 2);
        }
        if (_read_callback) {
          _read_callback(shared_from_this(), _read_buffer);
        }

        read();
      });
}

static auto handleWriteError(const auto& ec, auto id) -> int {
  if (ec) {
    LOG_ERROR("Session ID: {}. Write error: {}.", id, ec.message());
    return 1;
  }

  return 0;
}

auto Session::write() -> void {
  auto self = shared_from_this();

  auto still_writing = !_write_buffer.empty();
  if (!still_writing) {
    _write_buffer.resize(
        Buffer::DEFAULT_SIZE);  // avoid buffer from bigging too much
    {
      std::scoped_lock lock(_mutex);
      while (!_unsent_buffers.empty()) {
        auto& buffer = _unsent_buffers.front();
        _write_buffer.append(buffer.readBegin(), buffer.readableBytes());
        _unsent_buffers.pop();
      }
    }
  }

  socket().async_write_some(
      asio::buffer(_write_buffer.readBegin(), _write_buffer.readableBytes()),
      [self, this](const auto& ec, auto len) {
        if (handleWriteError(ec, _id) != 0) {
          disconnect();
          return;
        }

        _write_buffer.retrieve(len);
        if (_write_buffer.empty()) {
          return;
        }

        write();
      });
}

}  // namespace fz::net
