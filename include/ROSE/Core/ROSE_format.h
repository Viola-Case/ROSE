/**

  @file      ROSE_format.h
  @brief     Printf-style formatting and printing using C++20 std::format syntax
  @details   Format() builds a ROSE::String from a format string and arguments.
             PrintF() formats and writes directly to stdout.
             Both families accept the same {}-placeholder syntax as std::format.
  @author    Viola Case
  @date      10.03.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_stdlib.h>
#include <ROSE/Core/ROSE_rtl.h>


namespace ROSE {

  /** @brief Type-erased PrintF overload called by the variadic template. @internal **/
  void PrintF(const StringView &fmt_str, const std::format_args args);
  /** @brief Type-erased PrintF overload called by the variadic template. @internal **/
  void PrintF(const char *fmt_str, const std::format_args args);

  /**
    @brief   Formats arguments into a ROSE::String using std::format syntax.
    @tparam  Args     Argument types (deduced).
    @param   fmt_str  Format string with {}-placeholders.
    @param   args     Arguments to substitute.
    @retval  Formatted string.
  **/
  template<typename ...Args>
  String Format(const StringView &fmt_str, Args &&...args) {
    auto result = std::vformat(fmt_str.c_str(), std::make_format_args(args...));
    return String(result.c_str());
  }

  /**
    @brief   Formats arguments into a ROSE::String using std::format syntax.
    @tparam  Args     Argument types (deduced).
    @param   fmt_str  Null-terminated format string with {}-placeholders.
    @param   args     Arguments to substitute.
    @retval  Formatted string.
  **/
  template<typename ...Args>
  String Format(const char *fmt_str, Args &&...args) {
    auto result = std::vformat(fmt_str, std::make_format_args(args...));
    return String(result.c_str());
  }

  /**
    @brief   Formats and prints to stdout using std::format syntax.
    @tparam  Args     Argument types (deduced).
    @param   fmt_str  Format string with {}-placeholders.
    @param   args     Arguments to substitute.
  **/
  template<typename ...Args>
  void PrintF(const StringView fmt_str, Args &&...args) {
    PrintF(fmt_str, std::format_args(std::make_format_args(args...)));
  }

  /**
    @brief   Formats and prints to stdout using std::format syntax.
    @tparam  Args     Argument types (deduced).
    @param   fmt_str  Null-terminated format string with {}-placeholders.
    @param   args     Arguments to substitute.
  **/
  template<typename ...Args>
  void PrintF(const char *fmt_str, Args &&...args) {
    PrintF(fmt_str, std::format_args(std::make_format_args(args...)));
  }

}
