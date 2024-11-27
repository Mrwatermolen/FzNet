#include "fz/net/loop.h"

namespace fz::net {

Loop::Loop() : _work{_io_context} {}

Loop::~Loop() {
  if (_thread.joinable()) {
    _thread.join();
  }
}

auto Loop::start() -> void {
  _thread = std::thread([this] { run(); });
}

auto Loop::stop() -> void {
  _io_context.stop();
  if (_thread.joinable()) {
    _thread.join();
  }
}

auto Loop::postTask(std::function<void(void)> func) -> void {
  asio::post(_io_context, func);
}

auto Loop::run() -> void { _io_context.run(); }

}  // namespace fz::net
