/**

  @file      ROSE_quaternion.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/preamble/ROSE_stdlib.h>

#include <ROSE/Core/math/ROSE_complex.h>
#include <ROSE/Core/math/ROSE_vector.h>

namespace ROSE::math {
  enum class EulerOrder {
    ZXY,
    ZXZ,
    XYZ,
    XZY,
    YXZ,
    YZX,
    ZYX,
    XYX,
    XZX,
    YXY,
    YZY,
    ZYZ
  };

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

    template<typename T>
    static constexpr Quat<T> AxisAngle(T angle, T ax, T ay, T az) {
      T half = angle * T(0.5);
      T s = std::sin(half);
      return {
          std::cos(half),
          ax * s,
          ay * s,
          az * s
      };
    }

    template<typename T>
    static constexpr Quat<T> FromEuler(Vec<T, 3> v, EulerOrder order = EulerOrder::ZXY) {
      Quat<T> qx = AxisAngle(v.x, T(1), T(0), T(0));
      Quat<T> qy = AxisAngle(v.y, T(0), T(1), T(0));
      Quat<T> qz = AxisAngle(v.z, T(0), T(0), T(1));

      switch (order) {
      case EulerOrder::XYZ: return qx * qy * qz;
      case EulerOrder::XZY: return qx * qz * qy;

      case EulerOrder::YXZ: return qy * qx * qz;
      case EulerOrder::YZX: return qy * qz * qx;

      case EulerOrder::ZXY: return qz * qx * qy;
      case EulerOrder::ZYX: return qz * qy * qz; // careful — typo bait

      case EulerOrder::XYX: return qx * qy * qx;
      case EulerOrder::XZX: return qx * qz * qx;

      case EulerOrder::YXY: return qy * qx * qy;
      case EulerOrder::YZY: return qy * qz * qy;

      case EulerOrder::ZXZ: return qz * qx * qz;
      case EulerOrder::ZYZ: return qz * qy * qz;
      }

      return Quat<T>{1, 0, 0, 0}; // fallback identity
    }

    static constexpr Quat<T> Identity() {
      return Quat<T>{1, 0, 0, 0};
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