/**

  @file      quaternion.h
  @brief
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/stdlib.h>

#include <ROSE/Core/math/complex.h>
#include <ROSE/Core/math/vector.h>

namespace ROSE::math {
  enum class EulerOrder {
    XYZ,
    XZY,
    YXZ,
    YZX,
    ZXY,
    ZYX
  };

  /*!
   * @brief Quaternion
   * @tparam T - Underlying arithmetic type
   */
  template <StdScalar T>
  struct Quat {
    union {
      struct {
        T w, x, y, z;
      };
      T data[4];
    };
    constexpr Quat() noexcept : w(1), x(0), y(0), z(0) {}
    constexpr Quat(T _w, T _x = T {}, T _y = T {}, T _z = T {}) noexcept : w(_w), x(_x), y(_y), z(_z) {}
    constexpr Quat(Comp<T> _c, T _y = T {}, T _z = T {}) noexcept : w(_c.Re), x(_c.Im), y(_y), z(_z) {}
    constexpr Quat(const Quat &rhs) noexcept = default;
    constexpr explicit Quat(Vec4<T> vec) : w(vec.w), x(vec.x), y(vec.y), z(vec.z) {}

    static constexpr Quat AxisAngle(T angle, T ax, T ay, T az) {
      T half = angle * T(0.5);
      T s = std::sin(half);
      return {
        std::cos(half),
        ax * s,
        ay * s,
        az * s
      };
    }

    static constexpr Quat FromEuler(Vec<T, 3> v, EulerOrder order = EulerOrder::XYZ) {
      Quat<T> qx = AxisAngle(v.x, T(1), T(0), T(0));
      Quat<T> qy = AxisAngle(v.y, T(0), T(1), T(0));
      Quat<T> qz = AxisAngle(v.z, T(0), T(0), T(1));

      switch (order) {
      case EulerOrder::XYZ:
        return qx * qy * qz;
      case EulerOrder::XZY:
        return qx * qz * qy;

      case EulerOrder::YXZ:
        return qy * qx * qz;
      case EulerOrder::YZX:
        return qy * qz * qx;

      case EulerOrder::ZXY:
        return qz * qx * qy;
      case EulerOrder::ZYX:
        return qz * qy * qx;
      }

      return Quat<T> { 1, 0, 0, 0 }; // fallback identity
    }

    static constexpr Quat<T> Identity() {
      return Quat<T> { 1, 0, 0, 0 };
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

    constexpr Quat operator*(const Quat &rhs) const noexcept {
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

} // namespace ROSE::math
