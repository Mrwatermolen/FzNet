#include <asio.hpp>
#include <cstdint>
#include <iostream>
#include <string>

using asio::buffer;
using asio::dynamic_buffer;
using asio::io_context;
using asio::read_until;
using asio::write;
using asio::ip::tcp;

int main(int argc, char* argv[]) {
  std::string host = "localhost";
  uint16_t port = 2314;

  if (argc > 2) {
    host = argv[1];
    port = std::stoi(argv[2]);
  }

  io_context io_context{};
  auto resolver = tcp::resolver{io_context};
  auto endpoints = resolver.resolve(host, std::to_string(port));
  auto v4 = std::ranges::find_if(endpoints, [](const auto& result) {
    return result.endpoint().address().is_v4();
  });

  if (v4 == endpoints.end()) {
    std::cerr << "No IPv4 address found for " << host << ':' << port << '\n';
    return 1;
  }

  auto socket = tcp::socket{io_context};
  socket.connect(v4->endpoint());

  while (true) {
    std::cout << "Enter message: (q to quit) ";
    std::string message;
    std::getline(std::cin, message);

    if (message == "q") {
      break;
    }

    message += '\n';
    write(socket, buffer(message));
    message = "";
    read_until(socket, dynamic_buffer(message), '\n');
    std::cout << "Server echo: " << message;
  }

  return 0;
}
