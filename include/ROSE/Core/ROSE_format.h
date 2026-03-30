/**

  @file      ROSE_format.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      10.03.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_preamble.h>


namespace ROSE {
  template<typename ...Args>
  String Format(const StringView &fmt_str, Args &&...args) {
    auto result = std::vformat(fmt_str.c_str(), std::make_format_args(args...));
    return String(result.c_str());
  }

  template<typename ...Args>
  String Format(const char *fmt_str, Args &&...args) {
    auto result = std::vformat(fmt_str, std::make_format_args(args...));
    return String(result.c_str());
  }

  template<typename ...Args>
  WString Format(const WStringView &fmt_str, Args &&...args) {
    auto result = std::vformat(fmt_str, std::make_format_args(args...));
    return WString(result.c_str());
  }

  template<typename ...Args>
  WString Format(const char16_t *fmt_str, Args &&...args) {
    auto result = std::vformat(fmt_str, std::make_format_args(args...));
    return WString(result.c_str());
  }

}