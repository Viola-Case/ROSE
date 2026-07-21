/**

    @file      log.h
    @brief
    @details   ~
    @author    Viola Case
    @date      12.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <cstdint>
#include <ROSE/Core/format.h>

// todo make custom sinks
// todo make debug a runtime flag instead of preprocessor

namespace ROSE {
  enum class LogLevel : uint8_t { Trace, Debug, Info, Warn, Error, Fatal };

  inline String LogLevelToString(const LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace:
#if defined(ROSE_LOGLEVELS_USE_NERDFONT_SYMBOLS)
      return "󰍉";
#else
      return "[  TRACE  ]";
#endif
    case LogLevel::Debug:
#if defined(ROSE_LOGLEVELS_USE_NERDFONT_SYMBOLS)
      return "";
#else
      return "[    D    ]";
#endif
    case LogLevel::Info:
#if defined(ROSE_LOGLEVELS_USE_NERDFONT_SYMBOLS)
      return "";
#else
      return "[    i    ]";
#endif
    case LogLevel::Warn:
#if defined(ROSE_LOGLEVELS_USE_NERDFONT_SYMBOLS)
      return "";
#else
      return "[ WARNING ]";
#endif
    case LogLevel::Error:
#if defined(ROSE_LOGLEVELS_USE_NERDFONT_SYMBOLS)
      return "";
#else
      return "[  ERROR  ]";
#endif
    case LogLevel::Fatal:
#if defined(ROSE_LOGLEVELS_USE_NERDFONT_SYMBOLS)
      return "󰚌";
#else
      return "[!!FATAL!!]";
#endif
    default:
#if defined(ROSE_LOGLEVELS_USE_NERDFONT_SYMBOLS)
      return " ";
#else
      return "           ";
#endif
    }
  }

  template <typename... Args>
  inline void Log(LogLevel level, const char *format, Args &&...args) {
    String colorcode { "" };
    switch (level) {
    case LogLevel::Trace:
#if !(defined(_DEBUG) || defined(ROSE_DEBUG))
      return;
#endif
      break;
    case LogLevel::Debug:
#if !(defined(_DEBUG) || defined(ROSE_DEBUG))
      return;
#endif
      break;
    case LogLevel::Info:
      colorcode = "\033[36m";
    case LogLevel::Warn:
      colorcode = "\033[33m";
    case LogLevel::Error:
      colorcode = "\033[31m";
    case LogLevel::Fatal:
      colorcode = "\033[1;31m";
    default:
      break;
    }
    PrintF("{} \033[0m", LogLevelToString(level));
    PrintF(format, args...);
    if (level >= LogLevel::Info) PrintF("\033[0m");
  }

} // namespace ROSE
#if defined(_DEBUG) || defined(ROSE_DEBUG)
  #define ROSE_LOG_DEBUG(...)                                                                                          \
    { ROSE::Log(ROSE::LogLevel::Debug, __VA_ARGS__); }
  #define ROSE_LOG_TRACE(...)                                                                                          \
    { ROSE::Log(ROSE::LogLevel::Trace, __VA_ARGS__); }
#else
  #define ROSE_LOG_DEBUG(...) ((void)0)
  #define ROSE_LOG_TRACE(...) ((void)0)
#endif
#define ROSE_LOG_INFO(...)                                                                                             \
  { ROSE::Log(ROSE::LogLevel::Info, __VA_ARGS__); }
#define ROSE_LOG_WARN(...)                                                                                             \
  { ROSE::Log(ROSE::LogLevel::Warn, __VA_ARGS__); }
#define ROSE_LOG_ERROR(...)                                                                                            \
  { ROSE::Log(ROSE::LogLevel::Error, __VA_ARGS__); }
#define ROSE_LOG_FATAL(...)                                                                                            \
  { ROSE::Log(ROSE::LogLevel::Fatal, __VA_ARGS__); }
