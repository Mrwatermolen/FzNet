#ifndef __FZ_NET_LOOP_POOL_H__
#define __FZ_NET_LOOP_POOL_H__

#include <vector>

#include "fz/net/loop.h"

namespace fz::net {

class LoopPool {
 public:
  explicit LoopPool(std::size_t size);

  auto start() -> void;

  auto stop() -> void;

  auto findNext() -> std::shared_ptr<Loop>;

 private:
  std::vector<std::shared_ptr<Loop>> _loops;
};

}  // namespace fz::net

#endif  // __FZ_NET_LOOP_POOL_H__
