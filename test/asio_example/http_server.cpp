#include <asio.hpp>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "asio/ip/tcp.hpp"

static auto printThreadLog(std::string_view message) -> void {
  std::stringstream ss;
  ss << "Thread: " << std::this_thread::get_id() << " " << message << '\n';
  std::cout << ss.str();
}

constexpr static std::string_view CRLF = "\r\n";

class HttpRequest {
 public:
  enum class Method : std::uint8_t { Unkown, Get, Post, Put, Delete, Head };

  static constexpr auto methodFromString(std::string_view method) {
    if (method == "GET") {
      return HttpRequest::Method::Get;
    }

    if (method == "POST") {
      return HttpRequest::Method::Post;
    }

    if (method == "PUT") {
      return HttpRequest::Method::Put;
    }

    if (method == "DELETE") {
      return HttpRequest::Method::Delete;
    }

    if (method == "HEAD") {
      return HttpRequest::Method::Head;
    }

    return HttpRequest::Method::Unkown;
  }

  static constexpr auto methodToString(HttpRequest::Method method) {
    switch (method) {
      case HttpRequest::Method::Get:
        return "GET";
      case HttpRequest::Method::Post:
        return "POST";
      case HttpRequest::Method::Put:
        return "PUT";
      case HttpRequest::Method::Delete:
        return "DELETE";
      case HttpRequest::Method::Head:
        return "HEAD";
      default:
        return "UNKOWN";
    }
  }

  auto method() const { return _method; }

  auto setMethod(std::string_view method) {
    _method = methodFromString(method);
    return _method != HttpRequest::Method::Unkown;
  }

  auto setMethod(HttpRequest::Method method) { _method = method; }

  auto path() const { return _path; }

  auto setPath(std::string_view path) { _path = path; }

  auto& query() const { return _query; }

  auto addQuery(std::string_view key, std::string_view value) {
    _query[std::string{key}] = std::string{value};
  }

  auto& headers() const { return _headers; }

  auto addHeader(std::string_view key, std::string_view value) {
    _headers[std::string{key}] = std::string{value};
  }

  auto& body() const { return _body; }

  auto setBody(std::string_view body) { _body = body; }

  auto toString() const {
    std::stringstream ss;
    ss << methodToString(_method) << ' ' << _path;

    if (!_query.empty()) {
      ss << '?';
      for (const auto& [key, value] : _query) {
        ss << key << '=' << value << '&';
      }
      ss.seekp(-1, std::ios_base::end);
    }
    ss << ' ' << "HTTP/1.1" << CRLF;

    ss << "Connection: close" << CRLF;

    for (const auto& [key, value] : _headers) {
      ss << key << ": " << value << CRLF;
    }
    ss << CRLF;

    ss << _body;
    return ss.str();
  }

 private:
  Method _method;
  std::string _path;
  std::unordered_map<std::string, std::string> _query;
  std::unordered_map<std::string, std::string> _headers;
  std::string _body;
};

class HttpResponse {
 public:
  enum class Status : std::uint16_t {
    UNKOWN = 0,
    OK = 200,
    MovedPermanently = 301,
    BadRequest = 400,
    NotFound = 404,
  };

  constexpr static auto statusToString(HttpResponse::Status status) {
    switch (status) {
      case HttpResponse::Status::OK:
        return "OK";
      case HttpResponse::Status::MovedPermanently:
        return "Moved Permanently";
      case HttpResponse::Status::BadRequest:
        return "Bad Request";
      case HttpResponse::Status::NotFound:
        return "Not Found";
      default:
        return "Unkown";
    }
  }

  constexpr static auto statusFromString(std::string_view status) {
    if (status == "OK") {
      return HttpResponse::Status::OK;
    }

    if (status == "Moved Permanently") {
      return HttpResponse::Status::MovedPermanently;
    }

    if (status == "Bad Request") {
      return HttpResponse::Status::BadRequest;
    }

    if (status == "Not Found") {
      return HttpResponse::Status::NotFound;
    }

    return HttpResponse::Status::UNKOWN;
  }

  auto status() const { return _status; }

  auto setStatus(std::string_view status) {
    _status = statusFromString(status);
    return _status != HttpResponse::Status::UNKOWN;
  }

  auto setStatus(HttpResponse::Status status) { _status = status; }

  auto& headers() const { return _headers; }

  auto addHeader(std::string_view key, std::string_view value) {
    _headers[std::string{key}] = std::string{value};
  }

  auto& body() const { return _body; }

  auto setBody(std::string_view body) { _body = body; }

  auto toString() const {
    std::stringstream ss;
    ss << "HTTP/1.1 " << static_cast<std::uint16_t>(_status) << ' '
       << statusToString(_status) << CRLF;

    ss << "Connection: close" << CRLF;

    for (const auto& [key, value] : _headers) {
      ss << key << ": " << value << CRLF;
    }
    ss << CRLF;

    ss << _body;
    return ss.str();
  }

 private:
  std::unordered_map<std::string, std::string> _headers;
  Status _status;
  std::string _body;
};

class HttpSession : public std::enable_shared_from_this<HttpSession> {
 public:
  explicit HttpSession(asio::ip::tcp::socket socket)
      : _socket{std::move(socket)} {}

  auto& socket() { return _socket; }

  auto& socket() const { return _socket; }

  auto start() {
    auto self = shared_from_this();
    asio::async_read_until(
        socket(), asio::dynamic_buffer(_read_buffer), "\r\n\r\n",
        [this, self](const auto& ec, auto size) {
          if (ec) {
            std::cerr << "Server error: " << ec.message() << " in start.\n";
            return;
          }

          std::string_view request{_read_buffer.data(), size};
          std::string_view method = request.substr(0, request.find(' '));
          std::string_view path = request.substr(method.size() + 1);
          path = path.substr(0, path.find(' '));

          std::stringstream ss;
          ss << "Communication: " << socket().remote_endpoint().address() << ':'
             << socket().remote_endpoint().port() << " in start. ";
          ss << "Server received: " << size << " bytes. Request: " << request;
          printThreadLog(ss.str());

          if (method == "GET") {
            if (path == "/") {
              HttpResponse response;
              response.setStatus(HttpResponse::Status::OK);
              response.addHeader("Content-Type", "text/plain");
              response.addHeader("Content-Length", "12");
              response.setBody("Hello World!");
              auto response_str = response.toString();
              asio::async_write(
                  socket(), asio::buffer(response_str),
                  [this, self](const auto& ec, auto size [[maybe_unused]]) {
                    if (ec) {
                      std::cerr << "Server error: " << ec.message()
                                << " in start.\n";
                      return;
                    }

                    std::stringstream ss;
                    ss << "Communication: "
                       << socket().remote_endpoint().address() << ':'
                       << socket().remote_endpoint().port() << " in start. ";
                    printThreadLog(ss.str());
                  });
            } else {
              std::string response =
                  "HTTP/1.1 404 Not Found\r\n"
                  "Content-Type: text/plain\r\n"
                  "Content-Length: 9\r\n"
                  "\r\n"
                  "Not Found!";
              asio::async_write(socket(), asio::buffer(response),
                                [this, self, response](const auto& ec, auto size
                                                       [[maybe_unused]]) {
                                  if (ec) {
                                    std::cerr
                                        << "Server error: " << ec.message()
                                        << " in start.\n";
                                    return;
                                  }

                                  std::stringstream ss;
                                  ss << "Communication: "
                                     << socket().remote_endpoint().address()
                                     << ':' << socket().remote_endpoint().port()
                                     << " in start. ";
                                  printThreadLog(ss.str());
                                });
            }
          }
        });
  }

 private:
  asio::ip::tcp::socket _socket;
  std::string _write_buffer;
  std::string _read_buffer;
};

class HttpServer {
 public:
  HttpServer(asio::io_context& io_context, uint16_t port)
      : _acceptor_v4{io_context,
                     asio::ip::tcp::endpoint{asio::ip::tcp::v4(), port}} {
    _acceptor_v4.listen();
  }

  auto start() -> void {
    _acceptor_v4.async_accept([this](const auto& ec, auto socket) {
      if (ec) {
        std::cerr << "Server error: " << ec.message() << " in start.\n";
        return;
      }

      auto c = std::make_shared<HttpSession>(std::move(socket));
      c->start();
      start();
    });
  }

 private:
  asio::ip::tcp::acceptor _acceptor_v4;
  std::unordered_map<std::string, std::function<void()>> _routers;
};

int main() {
  asio::io_context io_context;
  HttpServer server{io_context, 80};
  server.start();
  io_context.run();
}
