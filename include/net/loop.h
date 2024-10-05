#ifndef __FZ_NET_LOOP_H__
#define __FZ_NET_LOOP_H__

#include <asio.hpp>
#include <functional>
#include <thread>

namespace fz::net {

class Loop {
 public:
  Loop();

  Loop(const Loop &) = delete;

  Loop &operator=(const Loop &) = delete;

  Loop(Loop &&) noexcept = delete;

  Loop &operator=(Loop &&) = delete;

  ~Loop();

  auto start() -> void;

  auto stop() -> void;

  auto postTask(std::function<void(void)>) -> void;

  auto &getIoContext() { return _io_context; }

 private:
  std::thread _thread;
  asio::io_context _io_context;
  asio::io_context::work _work;

  auto run() -> void;
};

}  // namespace fz::net

#endif  // __FZ_NET_LOOP_H__
