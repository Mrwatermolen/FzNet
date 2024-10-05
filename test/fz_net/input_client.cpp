#include <cstdint>
#include <iostream>
#include <string>

#include "asio/io_context.hpp"
#include "net/loop.h"
#include "net/tcp_client.h"

int main(int argc, char *argv[]) {
  std::string ip = "127.0.0.1";
  std::uint16_t port = 2314;

  if (2 < argc) {
    ip = argv[1];
    port = std::stoi(argv[2]);
  }

  std::cout << "Ctrl+C to exit\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  auto client = std::make_shared<fz::net::TcpClient>(
      std::make_shared<fz::net::Loop>(), ip, port);
  client->setConnectCallback([&client](const auto &session) {
    std::cout << std::this_thread::get_id() << " Connect to "
              << session->socket().remote_endpoint().address().to_string()
              << ":" << session->socket().remote_endpoint().port() << "\n";

    auto t = std::thread([session, &client] {
      while (true) {
        std::cout << std::this_thread::get_id() << " Input message: \n";
        std::string line;
        std::getline(std::cin, line);
        if (line == "exit") {
          break;
        }

        auto buffer = fz::net::Buffer{};
        buffer.append(line);
        session->send(buffer);
      }

      std::cout << std::this_thread::get_id() << " Disconnect from "
                << session->socket().remote_endpoint().address().to_string()
                << ":" << session->socket().remote_endpoint().port() << "\n";
      session->disconnect();
      client->stop();
    });

    t.detach();
  });

  client->setReadCallback([](const auto &session, auto &&buffer) {
    auto str = buffer.retrieveAllAsString();
    std::cout << std::this_thread::get_id() << " Receive: " << str << "\n";
  });

  client->setDisconnectCallback([&client](const auto &session) {
    if (session->socket().is_open()) {
      std::cout << std::this_thread::get_id() << " Disconnect from "
                << "Session ID: " << session->id()
                << ". Remote: " << session->remoteIp() << ":"
                << session->remotePort() << "\n";

      return;
    }

    std::cout << std::this_thread::get_id()
              << " Disconnect invalid Session ID: " << session->id();
  });

  client->connect();

  client->run();  // run as deamon

  // Make sure main thread is running
  asio::io_context io_context;
  asio::signal_set signals{io_context, SIGINT, SIGTERM};
  signals.async_wait([&io_context, &client](const auto &, int) {
    client->stop();
    io_context.stop();
  });
  io_context.run();

  return 0;
}