#include <asio.hpp>
#include <iostream>
#include <string>

int main() {
  uint16_t port = 2314;

  auto io_context = asio::io_context{};

  auto endpoint =
      asio::ip::tcp::endpoint{asio::ip::tcp::v4(), port};

  auto acceptor = asio::ip::tcp::acceptor{io_context, endpoint};

  std::cout << "Server is running on port " << port << '\n';

  while (true) {
    auto socket = asio::ip::tcp::socket{io_context};
    std::cout << "Waiting for connection...\n";
    acceptor.accept(socket);
    std::cout << "Connection established!\n";
    std::cout << socket.remote_endpoint().address().to_string() << '\n';
    std::cout << socket.remote_endpoint().port() << '\n';
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Sending message...\n";
    std::string message = "Hello, World!";
    std::vector<char> buffer(message.begin(), message.end());
    asio::write(socket,
                       asio::buffer(buffer.data(), buffer.size()));
    std::cout << "Message sent!\n";
  }

  io_context.run();
  return 0;
}