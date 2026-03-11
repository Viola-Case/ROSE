/**

  @file      ROSE_mathfunctions.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      11.03.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/preamble/ROSE_typetraits.h>

namespace ROSE::math {
  template<StdScalar T>
  constexpr T clamp(T value, T min, T max) noexcept {
    return (value > max ? max : (value < min ? min : value));
  }
}