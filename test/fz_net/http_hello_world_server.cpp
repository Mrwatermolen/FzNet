#include <asio.hpp>
#include <iostream>
#include <utility>

#include "asio/io_context.hpp"
#include "net/session.h"
#include "net/tcp_server.h"

constexpr inline std::string_view CRLF = "\r\n";
constexpr inline std::string_view COLON = ": ";
constexpr inline std::string_view SPACE = " ";

class HttpRequest {
 public:
  HttpRequest() { clear(); }

 public:
  enum Method : std::uint8_t { INVALID, GET, POST, PUT, DELETE, HEAD };

  enum Version : std::uint8_t { UNKNOWN, HTTP_1_0, HTTP_1_1 };

  constexpr static auto methodToString(Method method) -> std::string_view {
    switch (method) {
      case GET:
        return "GET";
      case POST:
        return "POST";
      case PUT:
        return "PUT";
      case DELETE:
        return "DELETE";
      case HEAD:
        return "HEAD";
      default:
        return "INVALID";
    }
  }

  constexpr static auto versionToString(Version version) -> std::string_view {
    switch (version) {
      case HTTP_1_0:
        return "HTTP/1.0";
      case HTTP_1_1:
        return "HTTP/1.1";
      default:
        return "UNKNOWN";
    }
  }

  constexpr static auto methodFromString(std::string_view method) -> Method {
    if (method == "GET") {
      return GET;
    }
    if (method == "POST") {
      return POST;
    }
    if (method == "PUT") {
      return PUT;
    }
    if (method == "DELETE") {
      return DELETE;
    }
    if (method == "HEAD") {
      return HEAD;
    }
    return INVALID;
  }

  constexpr static auto versionFromString(std::string_view version) -> Version {
    if (version == "HTTP/1.0") {
      return HTTP_1_0;
    }
    if (version == "HTTP/1.1") {
      return HTTP_1_1;
    }
    return UNKNOWN;
  }

 public:
  auto method() const { return _method; }

  auto setMethod(Method method) { _method = method; }

  auto path() const { return _path; }

  auto setPath(std::string_view path) { _path = path; }

  auto& querys() const { return _querys; }

  auto addQuery(std::string_view key, std::string_view value) {
    _querys.emplace(key, value);
  }

  auto version() const { return _version; }

  auto setVersion(Version version) { _version = version; }

  auto& headers() const { return _headers; }

  auto addHeader(std::string_view key, std::string_view value) {
    _headers[std::string{key}] = std::string{value};
  }

  auto& body() const { return _body; }

  auto setBody(std::string_view body) { _body = body; }

  auto keepAlive() const -> bool {
    auto connection = _headers.find("Connection");
    if (connection == _headers.end()) {
      return false;
    }
    return connection->second == "keep-alive";
  }

  auto clear() -> void {
    _method = INVALID;
    _path.clear();
    _querys.clear();
    _version = UNKNOWN;
    _headers.clear();
    _body.clear();
  }

  auto toString() const -> std::string {
    std::stringstream ss;
    ss << methodToString(_method) << SPACE << _path;
    if (!_querys.empty()) {
      ss << "?";
      for (const auto& [key, value] : _querys) {
        ss << key << "=" << value << "&";
      }
      ss.seekp(-1, std::ios_base::end);
    }

    ss << SPACE << versionToString(_version) << CRLF;

    for (const auto& [key, value] : _headers) {
      ss << key << COLON << value << CRLF;
    }
    ss << CRLF;

    ss << _body;

    return ss.str();
  }

  auto parseRequestLine(std::string_view data) -> std::string::size_type {
    // Request-Line = Method SP Request-URI SP HTTP-Version CRLF

    const auto pos = data.find(CRLF);
    if (pos == std::string::npos) {
      return pos;
    }

    auto request_line = data.substr(0, pos);

    const auto method_pos = request_line.find(SPACE);
    if (method_pos == std::string::npos) {
      return method_pos;
    }
    auto method = methodFromString(request_line.substr(0, method_pos));
    if (method == INVALID) {
      return method_pos;
    }
    setMethod(method);

    const auto url_pos = request_line.find(SPACE, method_pos + 1);
    if (url_pos == std::string::npos) {
      return url_pos;
    }

    auto path = request_line.substr(method_pos + 1, url_pos - method_pos - 1);
    const auto query_pos = path.find('?');
    if (query_pos != std::string::npos) {
      setPath(path.substr(0, query_pos));
      auto querys = path.substr(query_pos + 1);
      while (true) {
        auto equal_pos = querys.find('=');
        if (equal_pos == std::string::npos) {
          break;
        }

        auto key = querys.substr(0, equal_pos);
        querys.remove_prefix(equal_pos + 1);
        auto and_pos = querys.find('&');
        if (and_pos == std::string::npos) {
          addQuery(key, querys);
          break;
        }

        auto value = querys.substr(0, and_pos);
        querys.remove_prefix(and_pos + 1);
        addQuery(key, value);
      }
    } else {
      setPath(path);
    }

    auto version = versionFromString(request_line.substr(url_pos + 1));
    if (version == UNKNOWN) {
      return url_pos;
    }
    setVersion(version);

    return pos + CRLF.size();  // skip CRLF
  }

  auto parseOneHeader(std::string_view data) -> std::string::size_type {
    const auto pos = data.find(CRLF);
    if (pos == std::string::npos) {
      return pos;
    }
    auto header = data.substr(0, pos);
    if (header.empty()) {
      return pos;
    }

    const auto colon_pos = header.find(COLON);
    if (colon_pos == std::string::npos) {
      return colon_pos;
    }
    auto key = header.substr(0, colon_pos);
    auto value = header.substr(colon_pos + 2);
    addHeader(key, value);

    return pos + CRLF.size();
  }

  auto parseHeaders(std::string_view data) -> std::string::size_type {
    if (data.empty()) {
      return std::string::npos;
    }

    std::string::size_type pos = 0;
    while (true) {
      if (data.empty()) {
        break;
      }

      auto bytes = parseOneHeader(data);
      if (bytes == std::string::npos) {
        return bytes;
      }
      if (bytes == 0) {
        break;
      }

      pos += bytes;
      data.remove_prefix(bytes);
    }

    return pos;
  }

  auto parse(std::string_view data) -> bool {
    auto bytes = parseRequestLine(data);
    if (bytes == std::string::npos) {
      return false;
    }

    data.remove_prefix(bytes);
    bytes = parseHeaders(data);
    if (bytes == std::string::npos) {
      return false;
    }

    data.remove_prefix(bytes + CRLF.size());
    setBody(data);

    return true;
  }

 private:
  Method _method;
  std::string _path;
  std::unordered_map<std::string, std::string> _querys;
  Version _version;
  std::unordered_map<std::string, std::string> _headers;
  std::string _body;
};

class HttpRequestParse {
 public:
  constexpr static auto MAX_REQUEST_LINE_SIZE = 4096;

  enum class Status : std::uint8_t { INVALID, RequestLine, Headers, Body, OK };

  auto status() const { return _status; }

  auto& request() const { return _request; }

  auto& request() { return _request; }

  auto reset() {
    _status = Status::RequestLine;
    _request.clear();
    _data.clear();
    _body_size = std::numeric_limits<std::size_t>::max();
  }

  auto markAsInvalid() {
    _status = Status::INVALID;
    _request.clear();
    _data.clear();
    _body_size = std::numeric_limits<std::size_t>::max();
  }

  auto run(fz::net::Buffer& buffer) {
    if (buffer.empty()) {
      return;
    }

    if (status() == Status::INVALID || status() == Status::OK) {
      buffer.retrieve(buffer.readableBytes());  // clear buffer
      return;
    }

    _data += buffer.retrieveAllAsString();
    while (parse()) {
    }
  }

 private:
  auto parse() -> bool {
    switch (status()) {
      case Status::RequestLine: {
        const auto bytes = _request.parseRequestLine(_data);
        if (bytes == std::string::npos) {
          if (MAX_REQUEST_LINE_SIZE < _data.size()) {
            markAsInvalid();
          }

          return false;
        }

        _data.erase(0, bytes);
        _status = Status::Headers;
        return !_data.empty();
      }
      case Status::Headers: {
        const auto bytes = _request.parseHeaders(_data);
        if (bytes == std::string::npos) {
          return false;
        }

        if (bytes == 0) {
          _status = Status::Body;
          auto it = _request.headers().find("Content-Length");
          if (it != _request.headers().end()) {
            _body_size = std::stoul(it->second);
          } else {
            _body_size = 0;
          }

          return !_data.empty();
        }

        _data.erase(0, bytes);
        return !_data.empty();
      }
      case Status::Body: {
        if (_body_size == 0) {
          _status = Status::OK;
          return false;
        }

        if (!_data.empty() && _data.substr(0, 2) == CRLF) {
          _data.erase(0, 2);
        }

        if (_data.size() < _body_size) {
          return false;
        }

        _request.setBody(_data.substr(0, _body_size));
        _data.clear();
        _status = Status::OK;
        return false;
      }
      default:
        break;
    }

    return false;
  }

 private:
  Status _status{Status::RequestLine};
  HttpRequest _request;
  std::string _data;
  std::size_t _body_size{std::numeric_limits<std::size_t>::max()};
};

class HttpSession : public fz::net::Session {
 public:
  explicit HttpSession(std::shared_ptr<fz::net::Loop> loop)
      : fz::net::Session{std::move(loop)} {}

  auto parseRequest(fz::net::Buffer& buffer) -> void {
    _http_request_parse.run(buffer);
  }

  auto& httpRequestParse() const { return _http_request_parse; }

 private:
  HttpRequestParse _http_request_parse;
};

int main() {
  asio::io_context io_context;

  fz::net::TcpServer server{2, "0.0.0.0", 80};
  server.setNewSessionCallback<HttpSession>();
  server.setReadCallback([](const auto& session, auto& buffer) {
    auto http_session = std::dynamic_pointer_cast<HttpSession>(session);
    if (!http_session) {
      std::cerr << "dynamic_pointer_cast failed\n";
      std::terminate();
    }

    std::cout << "read callback\n";
    http_session->parseRequest(buffer);
    if (http_session->httpRequestParse().status() ==
        HttpRequestParse::Status::OK) {
      std::cout << http_session->httpRequestParse().request().toString()
                << '\n';

      auto http_response_buffer = fz::net::Buffer();
      http_response_buffer.append("HTTP/1.1 200 OK\r\n");
      http_response_buffer.append("Server: fz\r\n");
      http_response_buffer.append("Content-Length: 11\r\n");
      http_response_buffer.append("Content-Type: text/plain\r\n");
      http_response_buffer.append("\r\n");
      http_response_buffer.append("hello world");

      http_session->send(http_response_buffer);
    }
  }

  );

  server.start();

  asio::signal_set signals(io_context, SIGINT, SIGTERM);
  signals.async_wait([&](const auto&, const auto&) { server.stop(); });

  io_context.run();

  return 0;
}
