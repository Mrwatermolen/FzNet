#ifndef __FZ_NET_BUFFER_H__
#define __FZ_NET_BUFFER_H__

#include <cassert>
#include <cstddef>
#include <vector>

namespace fz::net {

class Buffer {
 public:
  static constexpr std::size_t DEFAULT_SIZE = 1024;

  [[nodiscard]] auto capacity() const { return _buffer.size(); }

  [[nodiscard]] auto empty() const { return _writer_pos == _reader_pos; }

  [[nodiscard]] auto full() const { return _writer_pos == _buffer.size(); }

  [[nodiscard]] auto writeableBytes() const { return _buffer.size() - _writer_pos; }

  [[nodiscard]] auto readableBytes() const { return _writer_pos - _reader_pos; }

  auto writeBegin() { return _buffer.data() + _writer_pos; }

  auto writeEnd() { return _buffer.data() + _buffer.size(); }

  auto readBegin() { return _buffer.data() + _reader_pos; }

  auto readEnd() { return _buffer.data() + _writer_pos; }

  [[nodiscard]] auto readBegin() const { return _buffer.data() + _reader_pos; }

  [[nodiscard]] auto readEnd() const { return _buffer.data() + _writer_pos; }

  auto hasWritten(std::size_t len) {
    _writer_pos += len;
    return _writer_pos;
  }

  auto append(const char* data, std::size_t len) {
    if (writeableBytes() < len) {
      auto new_size = std::max(_buffer.size() * 2, _buffer.size() + len);
      _buffer.resize(new_size);
    }

    std::copy(data, data + len, writeBegin());
    _writer_pos += len;
  }

  auto append(std::string_view data) { append(data.data(), data.size()); }

  auto append(const char* data) { append(data, std::strlen(data)); }

  auto append(char data) { append(&data, 1); }

  auto retrieve(std::size_t len) {
    if (readableBytes() <= len) {
      retrieveAll();
      return;
    }

    _reader_pos += len;
  }

  auto retrieveUntil(const char* end) {
    assert(readBegin() <= end);
    retrieve(end - readBegin());
  }

  auto retrieveAllAsString() {
    auto str = std::string{readBegin(), readableBytes()};
    retrieveAll();
    return str;
  }

  auto resize(std::size_t len) -> void {
    if (empty()) {
      retrieveAll();
      _buffer.resize(len);
      return;
    }

    if (len <= writeableBytes()) {
      return;
    }

    _buffer.resize(_buffer.size() + len);
  }

 private:
  auto retrieveAll() -> void {
    _reader_pos = 0;
    _writer_pos = 0;
  }

 private:
  std::size_t _writer_pos{};
  std::size_t _reader_pos{};
  std::vector<char> _buffer = std::vector<char>(DEFAULT_SIZE, 0);
};

}  // namespace fz::net

#endif  // __FZ_NET_BUFFER_H__
