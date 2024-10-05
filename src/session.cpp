#include "net/session.h"

#include <asio.hpp>
#include <cstddef>
#include <mutex>

#include "net/common/buffer.h"
#include "net/common/log.h"

namespace fz::net {

auto Session::start() -> void {
  _id = reinterpret_cast<std::uint64_t>(this);
  if (!socket().is_open()) {
    LOG_ERROR("Session ID: {}. Socket is not open.", _id);
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
  LOG_DEBUG("Session ID: {}. Remote: {}:{}. Disconnect.", _id, _remote_ip,
            _remote_port);
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
      LOG_ERROR(
          "Session ID: {}. Remote: {}:{}. error: {}. retry. Reconnect: "
          "{}. "
          "Remain "
          "reconnect times: {}.",
          _id, _remote_ip, _remote_port, ec.message(), _reconnect,
          _reconnect_times);

      if (!_reconnect || _reconnect_times <= 0) {
        LOG_DEBUG(
            "Session ID: {}. Remote: {}:{}. Reconnect {} or remain reconnect "
            "{} less than 0. Cancel reconnect.",
            _id, _remote_ip, _remote_port, _reconnect, _reconnect_times);

        this->disconnect();
        return;
      }

      _reconnect_times--;
      _socket.close();
      this->reconnect(_remote_ip, _remote_port);
      return;
    }

    LOG_DEBUG(
        "Session ID: {}. Remote: {}:{}. Connect done. Reconnect: {}. Remain "
        "reconnect times: {}.",
        _id, _remote_ip, _remote_port, _reconnect, _reconnect_times);
    start();
  });
}

auto Session::reconnect(const std::string& ip, std::uint16_t port) -> void {
  LOG_DEBUG("Session ID: {}. Remote: {}:{}. Reconnect pending.", _id, ip, port);

  auto self = shared_from_this();
  _timer.expires_after(asio::chrono::milliseconds(_reconnect_delay_ms));
  _timer.async_wait([this, self, ip, port](const auto& ec) {
    if (ec) {
      LOG_ERROR(
          "Session ID: {}. Remote: {}:{}. Reconnect error: {}. Remain "
          "reconnect times: {}.",
          _id, ip, port, ec.message(), _reconnect_times);
      return;
    }

    connect(ip, port, _reconnect);
  });
}

auto Session::send(const Buffer& buffer) -> void {
  auto self = shared_from_this();
  LOG_DEBUG("Session ID: {}. Remote: {}:{}. Send {} bytes.", _id, _remote_ip,
            _remote_port, buffer.readableBytes());

  {
    std::scoped_lock lock(_mutex);
    _unsent_buffers.push(buffer);
  }

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
          disconnect();
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
        if (ec) {
          LOG_ERROR("Session error: {}", ec.message());
          disconnect();
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
