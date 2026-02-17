/**

    @file      ROSE_typetraits.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      17.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/preamble/ROSE_stdlib.h>

namespace ROSE {
  template<typename T>
  concept Character =
    std::same_as<T, char> ||
    std::same_as<T, signed char> ||
    std::same_as<T, unsigned char> ||
    std::same_as<T, wchar_t> ||
    std::same_as<T, char8_t> ||
    std::same_as<T, char16_t> ||
    std::same_as<T, char32_t>;
}