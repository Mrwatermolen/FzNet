#include <asio.hpp>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
  std::string address = "127.0.0.1";
  uint16_t port = 2314;

  auto io_context = asio::io_context{};

  auto endpoint =
      asio::ip::tcp::endpoint{asio::ip::make_address(address), port};

  auto socket = asio::ip::tcp::socket{io_context};
  std::cout << "Connecting to server...\n";
  socket.connect(endpoint);

  std::array<char, 4096> buffer;
  asio::error_code error;

  std::cout << "Connected to server!\n";

  while (true) {
    size_t length = socket.read_some(asio::buffer(buffer), error);
    if (error) {
      if (error == asio::error::eof) {
        std::cout << "Getting EOF\n";
        std::cout << "Connection closed by server\n";
        break;
      }

      std::cout << "Error: " << error.message() << '\n';
      break;
    }

    std::cout << "Received message: ";
    std::cout << "Length: " << length << '\n';
    std::cout << std::string(buffer.data(), length) << '\n';
    if (error == asio::error::eof) {
      break;
    }

    if (error) {
      std::cerr << "Error: " << error.message() << '\n';
      break;
    }
  }

  return 0;
}