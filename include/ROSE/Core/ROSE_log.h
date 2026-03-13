/**

    @file      ROSE_log.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      12.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_preamble.h>

// todo figure out how this will actually work

namespace ROSE {
  enum class LogLevel :uint8_t {
    Trace,
    Info,
    Warn,
    Error,
    Fatal
  };

}

#define ROSE_LOG_TRACE(...)   //rose::Log(rose::LogLevel::Trace, __VA_ARGS__)
#define ROSE_LOG_INFO(...)    //rose::Log(ROSE::LogLevel::Info, __VA_ARGS__)
#define ROSE_LOG_WARN(...)    //rose::Log(ROSE::LogLevel::Warn, __VA_ARGS__)
#define ROSE_LOG_ERROR(...)   //rose::Log(ROSE::LogLevel::Error, __VA_ARGS__)
#define ROSE_LOG_FATAL(...)   //rose::Log(ROSE::LogLevel::Fatal, __VA_ARGS__)
