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
  template<Character CharT, typename ...Args>
  BasicString<CharT> Format(const BasicStringView<CharT> &fmt_str, Args &&...args) {
    auto result = fmt::format(fmt_str.c_str(), Forward<Args>(args)...);
    return BasicString<CharT>(result.c_str());
  }

}