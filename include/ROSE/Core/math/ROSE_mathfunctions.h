/**

  @file      ROSE_mathfunctions.h
  @brief
  @details   ~
  @author    Viola Case
  @date      11.03.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_typetraits.h>

namespace ROSE::math {
  template <StdScalar T>
  constexpr T Clamp(T value, T min, T max) noexcept {
    return (value > max ? max : (value < min ? min : value));
  }
  template <typename T>
  constexpr const T &Min(const T &a, const T &b) noexcept
    requires(std::is_arithmetic_v<T>)
  { return (b < a ? b : a); }
  template <typename T>
  constexpr const T &Max(const T &a, const T &b) noexcept
    requires(std::is_arithmetic_v<T>)
  { return (a < b ? b : a); }

} // namespace ROSE::math