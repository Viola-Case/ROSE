/**

  @file      format.h
  @brief
  @details   ~
  @author    Viola Case
  @date      10.03.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/macros.h>
#include <ROSE/Core/stdlib.h>
#include <ROSE/Core/rtl.h>


namespace ROSE {

  ROSE_API(Core) void PrintF(const StringView &fmt_str, const std::format_args args);
  ROSE_API(Core) void PrintF(const char *fmt_str, const std::format_args args);

  template <typename... Args>
  String Format(const StringView &fmt_str, Args &&...args) {
    auto result = std::vformat(fmt_str.c_str(), std::make_format_args(args...));
    return String(result.c_str());
  }

  template <typename... Args>
  String Format(const char *fmt_str, Args &&...args) {
    auto result = std::vformat(fmt_str, std::make_format_args(args...));
    return String(result.c_str());
  }


  /*!
   * Basically an implementation of std::println(std::format_string<Args...>, Args &&...args)
   */
  template <typename... Args>
  void PrintF(const StringView fmt_str, Args &&...args) {
    PrintF(fmt_str, std::format_args(std::make_format_args(args...)));
  }
  /*!
   * Basically an implementation of std::println(std::format_string<Args...>, Args &&...args)
   */
  template <typename... Args>
  void PrintF(const char *fmt_str, Args &&...args) {
    PrintF(fmt_str, std::format_args(std::make_format_args(args...)));
  }

} // namespace ROSE