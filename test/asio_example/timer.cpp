#include <asio.hpp>
#include <chrono>
#include <iostream>

void synchronousTimer() {
  std::cout << "synchronous_timer\n";
  asio::io_context ioc{};
  asio::steady_timer timer{ioc, std::chrono::seconds{5}};
  timer.wait();
  std::cout << "Hello, world!\n";
}

void asynchronousTimer() {
  std::cout << "asynchronous_timer\n";
  asio::io_context ioc{};
  asio::steady_timer timer{ioc, std::chrono::seconds{5}};
  auto printer{[](auto ec) { std::cout << "Hello, world!\n"; }};
  timer.async_wait(printer);
  std::cout << "After async_wait\n";
  ioc.run();
  std::cout << "After run\n";
}

void handler(auto&& ec, asio::steady_timer& timer, int counter) {
  if (counter < 5) {
    std::cout << "Hello, world!\n";
    ++counter;
    timer.expires_at(timer.expiry() + std::chrono::seconds{1});
    timer.async_wait([&timer, counter](auto&& PH1) {
      handler(std::forward<decltype(PH1)>(PH1), timer, counter);
    });
  }
}

void asynchronousTimerWithArg() {
  std::cout << "asynchronous_timer_with_arg\n";

  asio::io_context ioc{};
  asio::steady_timer timer{ioc, std::chrono::seconds{5}};
  int counter{0};
  timer.async_wait([&timer, counter](auto&& PH1) {
    handler(std::forward<decltype(PH1)>(PH1), timer, counter);
  });
  ioc.run();
}

int main() {
  synchronousTimer();
  asynchronousTimer();
  asynchronousTimerWithArg();
}