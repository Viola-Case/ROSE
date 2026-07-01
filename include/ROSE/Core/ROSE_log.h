/**

    @file      ROSE_log.h
    @brief
    @details   ~
    @author    Viola Case
    @date      12.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <cstdint>

// todo figure out how this will actually work

namespace ROSE {
  enum class LogLevel : uint8_t {
    Trace,
    Info,
    Warn,
    Error,
    Fatal
  };

  //inline void Log(LogLevel level, const char * format, ...) {
  //  ;
  //}

  #define Log(...) ((void)0)

}

#define ROSE_LOG_TRACE(...) //{ ROSE::Log(ROSE::LogLevel::Trace, __VA_ARGS__);}
#define ROSE_LOG_INFO(...)  //{ ROSE::Log(ROSE::LogLevel::Info, __VA_ARGS__);}
#define ROSE_LOG_WARN(...)  //{ ROSE::Log(ROSE::LogLevel::Warn, __VA_ARGS__);}
#define ROSE_LOG_ERROR(...) //{ ROSE::Log(ROSE::LogLevel::Error, __VA_ARGS__);}
#define ROSE_LOG_FATAL(...) //{ ROSE::Log(ROSE::LogLevel::Fatal, __VA_ARGS__);}
