/**

  @file      log.cpp
  @brief     
  @details   ~
  @author    Viola Case
  @date      12.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/ROSE.h>

#include <cstdio>

namespace ROSE {


  void LogV(LogLevel level, const char *fmt, va_list args) {
    vprintf(fmt, args);
  }

  void Log(LogLevel level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogV(level, fmt, args);
    va_end(args);
  }

  void LogTrace (const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogV(LogLevel::Trace, fmt, args);
    va_end(args);
  }
  void LogInfo  (const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogV(LogLevel::Info, fmt, args);
    va_end(args);
  }
  void LogWarn  (const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogV(LogLevel::Warn, fmt, args);
    va_end(args);
  }
  void LogError (const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogV(LogLevel::Error, fmt, args);
    va_end(args);
  }
  void LogFatal (const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogV(LogLevel::Fatal, fmt, args);
    va_end(args);
  }

  // todo figure out how this should work
  
}