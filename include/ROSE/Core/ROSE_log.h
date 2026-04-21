/**

    @file      ROSE_log.h
    @brief     Logging severity levels and compile-time log macros
    @details   The logging subsystem is not yet fully implemented; the macros
               are currently no-ops. LogLevel values are ordered from least
               to most severe.
    @author    Viola Case
    @date      12.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <cstdint>

// todo figure out how this will actually work

namespace ROSE {
  /**
    @enum  LogLevel
    @brief Severity levels for the ROSE logging subsystem.
  **/
  enum class LogLevel :uint8_t {
    Trace, //!< Fine-grained diagnostic messages (highest verbosity)
    Info,  //!< General informational messages
    Warn,  //!< Recoverable conditions that may indicate a problem
    Error, //!< Errors that impair functionality but do not halt execution
    Fatal  //!< Unrecoverable errors; execution should stop after logging
  };

}

#define ROSE_LOG_TRACE(...)   //rose::Log(rose::LogLevel::Trace, __VA_ARGS__)
#define ROSE_LOG_INFO(...)    //rose::Log(ROSE::LogLevel::Info, __VA_ARGS__)
#define ROSE_LOG_WARN(...)    //rose::Log(ROSE::LogLevel::Warn, __VA_ARGS__)
#define ROSE_LOG_ERROR(...)   //rose::Log(ROSE::LogLevel::Error, __VA_ARGS__)
#define ROSE_LOG_FATAL(...)   //rose::Log(ROSE::LogLevel::Fatal, __VA_ARGS__)
