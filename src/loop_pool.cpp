#include "net/loop_pool.h"

namespace fz::net {

LoopPool::LoopPool(std::size_t size) {
  _loops.reserve(size);
  for (std::size_t i = 0; i < size; ++i) {
    _loops.push_back(std::make_shared<Loop>());
  }
}

auto LoopPool::start() -> void {
  for (auto& loop : _loops) {
    loop->start();
  }
}

auto LoopPool::stop() -> void {
  for (auto& loop : _loops) {
    loop->stop();
  }
}

auto LoopPool::findNext() -> std::shared_ptr<Loop> {
  static std::size_t index = 0;
  return _loops[index++ % _loops.size()];
}

}  // namespace fz::net
