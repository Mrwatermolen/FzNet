#ifndef __FZ_NET_COMMON_LOG_H__
#define __FZ_NET_COMMON_LOG_H__

#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include <cstdint>

namespace fz::net::common {

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

class Log {
 public:
  enum class LogLevel : std::uint8_t {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    CRITICAL = 5
  };

  static auto init(LogLevel lv) {
    auto level = static_cast<spdlog::level::level_enum>(lv);
    spdlog::set_level(level);
    spdlog::set_pattern("[%D %H:%M:%S.%e][%L][%t][%s:%# %!] %^%v%$");
  }

 private:
};

#define LOG_TRACE(fmt, ...)                                                 \
  spdlog::log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::trace, \
              fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_DEBUG(fmt, ...)                                                 \
  spdlog::log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::debug, \
              fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_INFO(fmt, ...)                                                 \
  spdlog::log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::info, \
              fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_WARN(fmt, ...)                                                 \
  spdlog::log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::warn, \
              fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_ERROR(fmt, ...)                                               \
  spdlog::log({__FILENAME__, __LINE__, __FUNCTION__}, spdlog::level::err, \
              fmt __VA_OPT__(, ) __VA_ARGS__)

}  // namespace fz::net::common

#endif  // __FZ_NET_COMMON_LOG_H__
