#include <asio.hpp>
#include <cstdint>
#include <iostream>
#include <string>

static auto echo(asio::ip::tcp::socket socket) -> asio::awaitable<void> {
  try {
    for (;;) {
      auto buffer = std::vector<char>{};
      auto length = co_await asio::async_read_until(
          socket, asio::dynamic_buffer(buffer), '\n', asio::use_awaitable);
      if (length == 0) {
        break;
      }
      std::cout << "Server echo received: " << length
                << " bytes. Message: " << std::string(buffer.data(), length);

      co_await asio::async_write(socket, asio::buffer(buffer.data(), length),
                                 asio::use_awaitable);
    }

    std::cout << "Server echo connection closed\n";
  } catch (const asio::system_error& e) {
    std::cerr << "Server echo error: " << e.what() << '\n';
  }

  co_return;
}

static auto accept(asio::ip::tcp::acceptor& acceptor) -> void {
  asio::co_spawn(
      acceptor.get_executor(),
      [&acceptor]() -> asio::awaitable<void> {
        for (;;) {
          auto socket = co_await acceptor.async_accept(asio::use_awaitable);
          std::cout << "Server accepted connection from: "
                    << socket.remote_endpoint() << '\n';

          asio::co_spawn(acceptor.get_executor(), echo(std::move(socket)),
                         asio::detached);
        }
      },
      asio::detached);
}

int main(int argc, char* argv[]) {
  uint16_t port = 2314;
  if (argc > 1) {
    port = std::stoi(argv[1]);
  }

  asio::io_context io_context;
  asio::ip::tcp::acceptor acceptor{
      io_context, asio::ip::tcp::endpoint{asio::ip::tcp::v4(), port}};

  accept(acceptor);

  std::cout << "Server listening on port: " << port << '\n';

  io_context.run();

  return 0;
}
