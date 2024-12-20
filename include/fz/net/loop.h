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

  auto operator=(const Loop &) -> Loop & = delete;

  Loop(Loop &&) noexcept = delete;

  auto operator=(Loop &&) -> Loop & = delete;

  ~Loop();

  auto start() -> void;

  auto stop() -> void;

  auto postTask(std::function<void(void)>) -> void;

  auto getIoContext() -> auto & { return _io_context; }

 private:
  std::thread _thread;
  asio::io_context _io_context;
  asio::io_context::work _work;

  auto run() -> void;
};

}  // namespace fz::net

#endif  // __FZ_NET_LOOP_H__
