#include <cstdint>
#include <iostream>
#include <string>

#include "fz/net/session.h"
#include "fz/net/tcp_server.h"

int main(int argc, char *argv[]) {
  std::uint16_t port = 2314;

  if (1 < argc) {
    port = std::stoi(argv[1]);
  }

  asio::io_context io_context;

  fz::net::TcpServer server{1, "0.0.0.0", port};
  server.setNewSessionCallback<fz::net::Session>();
  server.setReadCallback([](const auto &session, auto &&buffer) {
    auto str = buffer.retrieveAllAsString();
    constexpr std::size_t max_show_size = 12;
    auto show_str = std::string{};
    if (max_show_size < str.size()) {
      show_str = str.substr(0, max_show_size) + "...";
    } else {
      show_str = str;
    }
    if (show_str.empty()) {
      return;
    }

    if (show_str.back() == '\n') {
      show_str.pop_back();
    }

    std::cout << "Receive: " << show_str << '\n';

    auto send = fz::net::Buffer{};
    send.append(str);
    session->send(send);
  });
  server.start();

  // asio listen to signal
  asio::signal_set signals{io_context, SIGINT, SIGTERM};
  signals.async_wait([&io_context, &server](const auto &, int) {
    server.stop();
    io_context.stop();
  });

  io_context.run();

  return 0;
}
