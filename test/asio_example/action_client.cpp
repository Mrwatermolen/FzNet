#include <algorithm>
#include <array>
#include <asio.hpp>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

class ActionClient {
 public:
  ActionClient(asio::io_context& io_context,
               const asio::ip::tcp::endpoint& endpoint)
      : _socket{io_context} {
    std::cout << "Connecting to server " << endpoint.address() << ':'
              << endpoint.port() << " ...\n";
    _socket.connect(endpoint);
  }

  auto start() -> void {
    if (!receiveHello()) {
      return;
    }

    doAction();
  }

  auto close() -> void {
    std::cout << "Closing connection...\n";
    try {
      _socket.close();
    } catch (const std::exception& e) {
      std::cerr << "Client error: " << e.what() << " in close.\n";
    }
  }

  auto& socket() { return _socket; }

  auto& socket() const { return _socket; }

 private:
  auto doAction() -> void {
    while (true) {
      std::cout << "Enter action: ";
      std::string line;
      std::getline(std::cin, line);

      if (line == "echo") {
        echo();
      } else if (line == "request") {
        request();

      } else if (line == "random") {
        random();
      } else {
        std::cerr << "Unknown action: " << line << '\n';
        break;
      }
    }

    close();
  }

  auto receiveHello() -> bool {
    std::cout << "Waiting for server hello...\n";
    auto buffer = std::vector<char>{};
    try {
      auto len = asio::read_until(socket(), asio::dynamic_buffer(buffer), '\n');
      if (len == 0) {
        std::cerr << "Client error: server hello not received.\n";
        close();
        return false;
      }
      std::cout << "Server hello: " << std::string(buffer.data(), len);

    } catch (const std::exception& e) {
      std::cerr << "Client error: " << e.what() << " in receiveHello.\n";
      close();
    }

    return true;
  }

  auto echo() -> void {
    auto header_buffer = std::array<char, 128>{};
    constexpr std::string_view header_str = "echo";
    std::ranges::copy(header_str, header_buffer.begin());
    asio::write(socket(), asio::buffer(header_buffer));
    std::cout << "Client waiting for echo accept...\n";
    auto len = asio::read(socket(), asio::buffer(header_buffer));
    constexpr std::string_view echo_accepted = "ECHO_ACCEPTED";
    auto accepted =
        std::string_view{header_buffer.data(), echo_accepted.size()};
    if (len == 0 || accepted != echo_accepted) {
      std::cerr << "Client error: echo not accepted. Received: "
                << echo_accepted << '\n';
      return;
    }

    std::cout << "Enter message to echo: ";
    std::string message;
    std::getline(std::cin, message);
    message += '\n';
    asio::write(socket(), asio::buffer(message));
    message = "";
    asio::read_until(socket(), asio::dynamic_buffer(message), '\n');
    std::cout << "Client echo: " << message;
  }

  auto request() -> void {
    auto header_buffer = std::array<char, 128>{};
    constexpr std::string_view header_str = "request";
    std::ranges::copy(header_str, header_buffer.begin());
    asio::write(socket(), asio::buffer(header_buffer));
    std::cout << "Client waiting for request accept...\n";
    auto len = asio::read(socket(), asio::buffer(header_buffer));
    constexpr std::string_view request_accepted = "REQUEST_ACCEPTED";
    auto accepted =
        std::string_view{header_buffer.data(), request_accepted.size()};
    if (len == 0 || accepted != request_accepted) {
      std::cerr << "Client error: request not accepted. Received: "
                << request_accepted << '\n';
      return;
    }

    std::cout << "Client request accepted\n";
    std::string command;
    asio::read_until(socket(), asio::dynamic_buffer(command), '\n');
    std::cout << "Client request: " << command;
  }

  auto random() -> void {
    int times = 5;
    while ((times--) != 0) {
      auto random =
          std::uniform_int_distribution<int>(200, 1000)(random_engine);
      std::this_thread::sleep_for(std::chrono::milliseconds(random));
      request();
    }
  }

 private:
  asio::ip::tcp::socket _socket;
  inline static std::mt19937 random_engine{std::random_device{}()};
};

int main(int argc, char* argv[]) {
  std::string host = "localhost";
  std::uint16_t port = 2314;

  if (argc > 2) {
    host = argv[1];
    port = std::stoi(argv[2]);
  }

  asio::io_context io_context;
  auto resolver = asio::ip::tcp::resolver{io_context};
  auto results = resolver.resolve(host, std::to_string(port));
  auto endpoint = *std::ranges::find_if(results, [](const auto& result) {
    return result.endpoint().address().is_v4();
  });
  auto client = ActionClient{io_context, endpoint};
  client.start();
}
