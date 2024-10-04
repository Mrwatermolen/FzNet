#include <sys/types.h>

#include <array>
#include <asio.hpp>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

static auto printThreadLog(std::string_view message) -> void {
  std::stringstream ss;
  ss << "Thread: " << std::this_thread::get_id() << " " << message << '\n';
  std::cout << ss.str();
}

class ActionServer : public std::enable_shared_from_this<ActionServer> {
 public:
  explicit ActionServer(asio::io_context& io_context) : _socket{io_context} {}

  auto& socket() { return _socket; }

  auto& socket() const { return _socket; }

  auto start() { sayHello(); }

  auto close() -> void {
    std::stringstream ss;
    ss << "Closing connection from: " << socket().remote_endpoint().address()
       << ':' << socket().remote_endpoint().port();
    printThreadLog(ss.str());
    try {
      socket().close();
    } catch (const std::exception& e) {
      std::cerr << "Server error: " << e.what() << " in close.\n";
    }
  }

 private:
  asio::ip::tcp::socket _socket;

  auto sayHello() -> void {
    auto self = shared_from_this();
    std::string message = "Hello from server!\n";

    asio::async_write(
        socket(), asio::buffer(message),
        [this, self](const auto& ec, auto size [[maybe_unused]]) {
          if (ec) {
            std::stringstream ss;
            ss << "Communication: " << socket().remote_endpoint().address()
               << ':' << socket().remote_endpoint().port() << " in sayHello. ";
            ss << "Server error: " << ec.message()
               << " in sayHello. Remote endpoint: "
               << socket().remote_endpoint().address() << ':'
               << socket().remote_endpoint().port() << "\n";
            std::cerr << ss.str();
            return;
          }
          doAction();
        });
  }

  auto doAction() -> void {
    auto self = shared_from_this();
    std::shared_ptr<std::array<char, 128>> header_buffer =
        std::make_shared<std::array<char, 128>>();
    asio::async_read(
        socket(), asio::buffer(*header_buffer),
        [header_buffer, this, self](const auto& ec, auto transform_size) {
          if (ec) {
            if (ec == asio::error::eof) {
              std::stringstream ss;
              ss << "Communication: " << socket().remote_endpoint().address()
                 << ':' << socket().remote_endpoint().port()
                 << " in doAction. ";
              ss << "Connection closed by client: "
                 << socket().remote_endpoint().address() << ':'
                 << socket().remote_endpoint().port();
              printThreadLog(ss.str());
              return;
            }

            std::stringstream ss;
            ss << "Server error: " << ec.message() << " in doAction.\n";
            std::cerr << ss.str();
            return;
          }

          std::stringstream ss;
          ss << "Communication: " << socket().remote_endpoint().address() << ':'
             << socket().remote_endpoint().port() << " in doAction. ";
          ss << "Server action received: " << transform_size;
          printThreadLog(ss.str());

          std::string action_str =
              std::string(header_buffer->data(), transform_size);

          ss.str("");
          ss << "Communication: " << socket().remote_endpoint().address() << ':'
             << socket().remote_endpoint().port() << " in doAction. ";
          ss << "Server action: " << action_str;
          printThreadLog(ss.str());

          if (action_str.substr(0, 4) == "echo") {
            echo();
          } else if (action_str.substr(0, 7) == "request") {
            request();
          } else {
            std::cerr << "Unknown action: " << action_str << '\n';
            close();
          }
        });
  }

  auto echo() -> void {
    auto self = shared_from_this();
    std::array<char, 128> header_buffer;
    std::string_view header_str = "ECHO_ACCEPTED";
    std::ranges::copy(header_str, header_buffer.begin());
    asio::async_write(
        socket(), asio::buffer(header_buffer),
        [this, self](const auto& ec, auto size) {
          if (ec) {
            std::cerr << "Server echo error: " << ec.message() << '\n';
            return;
          }

          {
            std::stringstream ss;
            ss << "Communication: " << socket().remote_endpoint().address()
               << ':' << socket().remote_endpoint().port() << " in echo. ";
            ss << "Server echo accepted: " << size << " bytes";
            printThreadLog(ss.str());
          }

          auto buffer = std::make_shared<std::vector<char>>();
          asio::async_read_until(
              socket(), asio::dynamic_buffer(*buffer), '\n',
              [buffer, this, self](const auto& ec, auto size) {
                if (ec) {
                  std::cerr << "Server echo error: " << ec.message() << '\n';
                  return;
                }

                std::string message = std::string(buffer->data(), size - 1);

                std::stringstream ss;
                ss << "Communication: " << socket().remote_endpoint().address()
                   << ':' << socket().remote_endpoint().port() << " in echo. ";
                ss << "Server echo received: " << size
                   << " bytes. Message: " << message;
                printThreadLog(ss.str());
                message += '\n';

                asio::async_write(
                    socket(), asio::buffer(message),
                    [this](const auto& ec, auto size) {
                      if (ec) {
                        std::cerr << "Server echo error: " << ec.message()
                                  << '\n';
                        return;
                      }

                      std::stringstream ss;
                      ss << "Communication: "
                         << socket().remote_endpoint().address() << ':'
                         << socket().remote_endpoint().port() << " in echo. ";
                      ss << "Server echo sent: " << size << " bytes";
                      printThreadLog(ss.str());
                    });

                doAction();
              });
        });
  }

  auto request() -> void {
    auto self = shared_from_this();
    constexpr std::string_view header_str = "REQUEST_ACCEPTED";
    std::array<char, 128> header_buffer;
    std::ranges::copy(header_str, header_buffer.begin());
    asio::async_write(
        socket(), asio::buffer(header_buffer),
        [this, self](const auto& ec, auto size) {
          if (ec) {
            std::cerr << "Server request error: " << ec.message() << '\n';
            return;
          }

          std::stringstream ss;
          ss << "Communication: " << socket().remote_endpoint().address() << ':'
             << socket().remote_endpoint().port() << " in request. ";
          ss << "Server request sent accept header: " << size << " bytes";
          printThreadLog(ss.str());

          size = std::uniform_int_distribution<std::size_t>(20,
                                                            128)(random_engine);
          std::string command = randomStr(size);
          ss.str("");
          ss << "Communication: " << socket().remote_endpoint().address() << ':'
             << socket().remote_endpoint().port() << " in request. ";
          ss << "Server request generated command: " << command;
          command += '\n';
          printThreadLog(ss.str());
          asio::async_write(
              socket(), asio::buffer(command),
              [this, self, command = std::move(command)](const auto& ec,
                                                         auto size) {
                if (ec) {
                  std::cerr << "Server request error: " << ec.message() << '\n';
                  return;
                }

                std::stringstream ss;
                ss << "Communication: " << socket().remote_endpoint().address()
                   << ':' << socket().remote_endpoint().port()
                   << " in request. ";
                ss << "Server request sent: " << size
                   << " bytes. Command: " << command.substr(0, size - 1);
                printThreadLog(ss.str());

                doAction();
              });
        });
  }

  auto randomStr(std::size_t length) -> std::string {
    constexpr std::string_view str =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<> distr(0, str.size() - 1);
    std::string result;
    result.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
      result.push_back(str[distr(random_engine)]);
    }
    return result;
  }

  inline static std::mt19937 random_engine{std::random_device{}()};
};

class ActionAcceptor {
 public:
  ActionAcceptor(asio::io_context& io_context, uint16_t port)
      : _io_context{io_context},
        _acceptor{io_context,
                  asio::ip::tcp::endpoint{asio::ip::tcp::v4(), port}} {
    std::stringstream ss;
    ss << "Server is running on port " << port;
    printThreadLog(ss.str());
  }

  auto accept() -> void {
    auto s = std::make_shared<ActionServer>(_io_context);
    _acceptor.async_accept(s->socket(), [s, this](const auto& ec) {
      if (ec) {
        std::cerr << "Accept error: " << ec.message() << '\n';
        s->close();
        std::terminate();
      }

      std::stringstream ss;
      ss << "New connection from: " << s->socket().remote_endpoint().address()
         << ':' << s->socket().remote_endpoint().port();
      printThreadLog(ss.str());

      s->start();

      this->accept();
    });
  }

 private:
  asio::io_context& _io_context;
  asio::ip::tcp::acceptor _acceptor;
};

int main(int argc, char* argv[]) {
  uint16_t port = 2314;
  if (argc > 1) {
    port = std::stoi(argv[1]);
  }

  printThreadLog("Starting server");

  auto io_context = asio::io_context{};

  ActionAcceptor acceptor{io_context, port};
  acceptor.accept();

  printThreadLog("Running io_context");
  io_context.run();

  return 0;
}