/**

  @file      ROSE_quaternion.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/math/ROSE_complex.h>

namespace ROSE::math {
  template<StdScalar T>
  struct Quat {
    union {
      struct {
        T w, x, y, z;
      };
      T data[4];
      struct {
        Comp<T> c;
        T y, z;
      };
    };
    constexpr Quat() = default;
    constexpr Quat(T _w, T _x = T{}, T _y = T{}, T _z = T{}) noexcept : w(_w), x(_x), y(_y), z(_z) {}
    constexpr Quat(Comp<T> _c, T _y = T{}, T _z = T{}) noexcept : c(_c), y(_y), z(_z) {}
  };
  
}