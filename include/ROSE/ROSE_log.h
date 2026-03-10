/**

    @file      ROSE_log.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      12.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/ROSE_preamble.h>

// todo figure out how this will actually work

namespace ROSE {
  enum class LogLevel :uint8_t {
    Trace,
    Info,
    Warn,
    Error,
    Fatal
  };

  using LogCallback = void(*)(LogLevel, const char *message);

  class LogSink {
  public:
    virtual void Write(LogLevel level, const char *message) = 0;
  };

  void Log(LogLevel level, const char *fmt, ...);
  void SetLogCallback(LogCallback callback);

  void LogTrace(const char *fmt, ...);
  void LogInfo(const char *fmt, ...);
  void LogWarn(const char *fmt, ...);
  void LogError(const char *fmt, ...);
  void LogFatal(const char *fmt, ...);

}

#define ROSE_TRACE(...)   //rose::Log(rose::LogLevel::Trace, __VA_ARGS__)
#define ROSE_INFO(...)    //rose::Log(ROSE::LogLevel::Info, __VA_ARGS__)
#define ROSE_WARN(...)    //rose::Log(ROSE::LogLevel::Warn, __VA_ARGS__)
#define ROSE_ERROR(...)   //rose::Log(ROSE::LogLevel::Error, __VA_ARGS__)
#define ROSE_FATAL(...)   //rose::Log(ROSE::LogLevel::Fatal, __VA_ARGS__)
