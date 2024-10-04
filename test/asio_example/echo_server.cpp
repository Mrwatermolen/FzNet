#include <asio.hpp>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using asio::async_read_until;
using asio::async_write;
using asio::buffer;
using asio::dynamic_buffer;
using asio::io_context;
using asio::ip::tcp;

static auto echo(const std::shared_ptr<tcp::socket>& socket_ptr) -> void {
  auto msg = std::make_shared<std::vector<char>>();
  async_read_until(
      *socket_ptr, dynamic_buffer(*msg), '\n',
      [&socket_ptr, msg](const auto& ec, auto size) {
        if (ec) {
          std::cerr << "Server echo read error: " << ec.message() << '\n';
          return;
        }

        std::cout << "Server echo received: " << size
                  << " bytes. Message: " << std::string(msg->data(), size);

        async_write(*socket_ptr, buffer(*msg), [](const auto& ec, auto size) {
          if (ec) {
            std::cerr << "Server echo write error: " << ec.message() << '\n';
            return;
          }

          std::cout << "Server echo sent: " << size << " bytes" << '\n';
        });

        echo(socket_ptr);
      });
}

auto accept(tcp::acceptor& acceptor) -> void {
  acceptor.async_accept([&acceptor](const auto& ec, auto socket) {
    if (ec) {
      std::cerr << "Server error: " << ec.message() << '\n';
      return;
    }

    std::cout << "Server accepted connection from: " << socket.remote_endpoint()
              << '\n';
    echo(std::make_shared<tcp::socket>(std::move(socket)));
    accept(acceptor);
  });
}

int main(int argc, char* argv[]) {
  uint16_t port = 2314;

  if (argc > 1) {
    port = std::stoi(argv[1]);
  }

  io_context io_context;
  auto acceptor = tcp::acceptor{io_context, tcp::endpoint{tcp::v4(), port}};
  std::cout << "Server is running on port " << port << '\n';
  accept(acceptor);

  io_context.run();
  return 0;
}