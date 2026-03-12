/**

  @file      ROSE_format.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      10.03.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/ROSE_preamble.h>
#pragma warning(push)
#pragma warning(disable: 4828)
#include <fmt/core.h>
#pragma warning(pop)
//template<>
//struct fmt::formatter<ROSE::String> {
//
//};

namespace ROSE {
  template<typename ...Args>
  String Format(const StringView &fmt_str, Args &&...args) {
    auto result = fmt::format(fmt::runtime(fmt_str.c_str()), Forward<Args>(args)...);
    return String(result.c_str());
  }

  template<typename ...Args>
  String Format(const char *fmt_str, Args &&...args) {
    auto result = fmt::format(fmt::runtime(fmt_str), Forward<Args>(args)...);
    return String(result.c_str());
  }

}