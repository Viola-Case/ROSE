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
#include <ROSE/math/ROSE_vector.h>

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

    constexpr const Quat FromEuler(Vec<T,3> vec) {

    }

    constexpr Quat &operator*=(const Quat &rhs) noexcept {
      const T nw = w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z;
      const T nx = w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y;
      const T ny = w * rhs.y + y * rhs.w + z * rhs.x - x * rhs.z;
      const T nz = w * rhs.z + z * rhs.w + x * rhs.y - y * rhs.x;

      w = nw;
      x = nx;
      y = ny;
      z = nz;
      
      return *this;
    }

    constexpr const Quat &operator*(const Quat &rhs) const noexcept {
      Quat result(*this);
      return (result *= rhs);
    }
    /*constexpr Quat &operator*(const Quat &rhs) const noexcept {
      Quat result(*this);
      return (result *= rhs);
    }*/

  };

  using Quatf = Quat<float>;
  using Quatd = Quat<double>;
  
}