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
#include <ROSE/Core/math/matrix.h>
#include <ROSE/Core/math/mathfunctions.h>

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
    constexpr explicit Quat(Vec4<T> vec) : w(vec[3]), x(vec[0]), y(vec[1]), z(vec[2]) {}

    /*!
     * @brief Quaternion from an axis and an angle in radians.
     *
     * Uses @ref ROSE::math::Sin / @ref ROSE::math::Cos rather than `std::sin`/`std::cos`, which are not `constexpr`
     * before C++26 and so used to make this `constexpr` in name only. Those wrappers branch on
     * `__builtin_is_constant_evaluated()`, so the runtime path is still the hardware intrinsic and only constant
     * evaluation takes the Taylor fallback.
     *
     * The angle is promoted to a floating-point type first: `T` may be integral, and the `Sin` overloads take `float`
     * or `double`, so an integral argument would otherwise be ambiguous.
     *
     * @warning The two paths need not agree in the last bit; see the note on @ref ROSE::math::Sin. Don't
     *          `static_assert` a folded component against a decimal literal.
     */
    static constexpr Quat AxisAngle(T angle, T ax, T ay, T az) {
      using F = std::conditional_t<std::is_same_v<T, float>, float, double>;

      const F half = static_cast<F>(angle) * F(0.5);
      const F s = Sin(half);

      return {
        static_cast<T>(Cos(half)),
        static_cast<T>(ax * s),
        static_cast<T>(ay * s),
        static_cast<T>(az * s)
      };
    }

    static constexpr Quat FromEuler(Vec3<T> v, EulerOrder order = EulerOrder::XYZ) {
      /* Indexed rather than named reads: Vec's constructors activate the `data` member of its union, and constant
       * evaluation will not read the inactive one. Same values, but this folds. */
      Quat<T> qx = AxisAngle(v[0], T(1), T(0), T(0));
      Quat<T> qy = AxisAngle(v[1], T(0), T(1), T(0));
      Quat<T> qz = AxisAngle(v[2], T(0), T(0), T(1));

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

    //todo finish
    Vec3<T> ToEuler(EulerOrder order = EulerOrder::ZYX) {
      switch (order) {
      case EulerOrder::XYZ:
        return {};
      case EulerOrder::XZY:
        return {};

      case EulerOrder::YXZ:
        return {};
      case EulerOrder::YZX:
        return {};

      case EulerOrder::ZXY:
        return {};
      case EulerOrder::ZYX:
        return {};
      }
      return Vec3<T>{}; // fallback
    }

    static constexpr Quat<T> Identity() {
      return Quat<T> { 1, 0, 0, 0 };
    }

    constexpr T Norm() const noexcept {
      return Sqrt(w * w + x * x + y * y + z * z);
    }

    /*!
     * Rescales to unit length in place. Only unit quaternions represent a
     * rotation, and repeated products drift off the unit sphere, so anything
     * accumulating rotations needs to renormalize periodically. A degenerate
     * quaternion collapses to identity rather than producing NaNs.
     */
    constexpr Quat &Normalize() noexcept {
      const T n = Norm();
      if (!(n > T(0))) return (*this = Identity());
      const T inv = T(1) / n;
      w *= inv;
      x *= inv;
      y *= inv;
      z *= inv;
      return *this;
    }

    constexpr Quat Normalized() const noexcept {
      Quat result(*this);
      return result.Normalize();
    }

    /*!
     * The equivalent rotation as a homogeneous 4x4 matrix, in the column-vector convention the
     * rest of the math library uses: `M * v` rotates a point, and `M` composes on the left.
     *
     * Assumes a unit quaternion - a drifted one produces a matrix that also scales. Call
     * `Normalized()` first if the quaternion has been accumulating products.
     */
    constexpr Mat4<T> ToMat4() const noexcept {
      const T xx = x * x, yy = y * y, zz = z * z;
      const T xy = x * y, xz = x * z, yz = y * z;
      const T wx = w * x, wy = w * y, wz = w * z;

      Mat4<T> m = Mat4<T>::Identity();
      m.data[0]  = T { 1 } - T { 2 } * (yy + zz);
      m.data[1]  = T { 2 } * (xy - wz);
      m.data[2]  = T { 2 } * (xz + wy);

      m.data[4]  = T { 2 } * (xy + wz);
      m.data[5]  = T { 1 } - T { 2 } * (xx + zz);
      m.data[6]  = T { 2 } * (yz - wx);

      m.data[8]  = T { 2 } * (xz - wy);
      m.data[9]  = T { 2 } * (yz + wx);
      m.data[10] = T { 1 } - T { 2 } * (xx + yy);
      return m;
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
