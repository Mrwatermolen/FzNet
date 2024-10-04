// #ifndef __FZ_NET_COMMON_LOG_H__
// #define __FZ_NET_COMMON_LOG_H__

#include <format>
#include <iostream>
#include <string>

namespace fz::net::common {

#define LOG_TRACE(fmt, ...)                                              \
  {                                                                      \
    auto __log_trace_value = std::format(fmt, __VA_ARGS__);              \
    std::format_to(std::ostream_iterator<char>(std::cout),               \
                   "[TRACE] {}:{}:{} {}\n", __FILE_NAME__, __FUNCTION__, \
                   __LINE__, __log_trace_value);                         \
  }

#define LOG_DEBUG(fmt, ...)                                              \
  {                                                                      \
    auto __log_debug_value = std::format(fmt, __VA_ARGS__);              \
    std::format_to(std::ostream_iterator<char>(std::cout),               \
                   "[DEBUG] {}:{}:{} {}\n", __FILE_NAME__, __FUNCTION__, \
                   __LINE__, __log_debug_value);                         \
  }

#define LOG_INFO(fmt, ...)                                              \
  {                                                                     \
    auto __log_info_value = std::format(fmt, __VA_ARGS__);              \
    std::format_to(std::ostream_iterator<char>(std::cout),              \
                   "[INFO] {}:{}:{} {}\n", __FILE_NAME__, __FUNCTION__, \
                   __LINE__, __log_info_value);                         \
  }

#define LOG_WARN(fmt, ...)                                              \
  {                                                                     \
    auto __log_warn_value = std::format(fmt, __VA_ARGS__);              \
    std::format_to(std::ostream_iterator<char>(std::cout),              \
                   "[WARN] {}:{}:{} {}\n", __FILE_NAME__, __FUNCTION__, \
                   __LINE__, __log_warn_value);                         \
  }

#define LOG_ERROR(fmt, ...)                                              \
  {                                                                      \
    auto __log_error_value = std::format(fmt, __VA_ARGS__);              \
    std::format_to(std::ostream_iterator<char>(std::cout),               \
                   "[ERROR] {}:{}:{} {}\n", __FILE_NAME__, __FUNCTION__, \
                   __LINE__, __log_error_value);                         \
  }

#define LOG_CRITICAL(fmt, ...)                                              \
  {                                                                         \
    auto __log_critical_value = std::format(fmt, __VA_ARGS__);              \
    std::format_to(std::ostream_iterator<char>(std::cout),                  \
                   "[CRITICAL] {}:{}:{} {}\n", __FILE_NAME__, __FUNCTION__, \
                   __LINE__, __log_critical_value);                         \
  }

}  // namespace fz::net::common

// #endif // __FZ_NET_COMMON_LOG_H__
