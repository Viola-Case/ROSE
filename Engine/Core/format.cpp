/**

  @file      format.cpp
  @brief
  @details
  @author
  @date
  @copyright © Viola Case, 2026. All rights reserved.

**/

#include <iostream>
#include <ROSE/Core/ROSE_format.h>
namespace ROSE {
  void PrintF(const StringView &fmt_str, std::format_args args) {
    PrintF(fmt_str.c_str(), args);
  }
  //void PrintF(const WStringView fmt_str, const std::wformat_args args) {
  //  PrintF(fmt_str.c_str(), args);
  //}
  void PrintF(const char *fmt_str, std::format_args args) {
    std::cout << std::vformat(fmt_str, args);
  }
  //void PrintF(const char16_t *fmt_str, const std::wformat_args args) {
  //  std::wcout << std::vformat(reinterpret_cast<const wchar_t *>(fmt_str), args);
  //}
}
