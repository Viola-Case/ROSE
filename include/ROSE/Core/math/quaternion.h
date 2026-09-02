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
    static constexpr Quat AxisAngle(T angle, Vec3<T> axis) {
      using F = std::conditional_t<std::is_same_v<T, float>, float, double>;

      const F half = static_cast<F>(angle) * F(0.5);
      const F s = Sin(half);

      return {
        static_cast<T>(Cos(half)),
        static_cast<T>(axis.x * s),
        static_cast<T>(axis.y * s),
        static_cast<T>(axis.z * s)
      };
    }

    static constexpr Quat FromEuler(Vec3<T> v, EulerOrder order = EulerOrder::XYZ) {
      /* Indexed rather than named reads: Vec's constructors activate the `data` member of its union, and constant
       * evaluation will not read the inactive one. Same values, but this folds. */
      Quat<T> qx = AxisAngle(v[0], {T(1), T(0), T(0)});
      Quat<T> qy = AxisAngle(v[1], {T(0), T(1), T(0)});
      Quat<T> qz = AxisAngle(v[2], {T(0), T(0), T(1)});

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

    /*!
     * How near the middle rotation may come to a pole before @ref ToEuler treats it as locked,
     * measured as the cosine of that angle.
     *
     * It is the crossover between two approximations, and it bounds the error of both. The
     * locked branch is exact *at* the pole and drifts linearly as you move off it; the general
     * branch is the reverse. So this is also the worst-case error of the whole function, in
     * radians, and it wants to be as small as the type's precision allows.
     *
     * The two costs run opposite ways, so the best threshold is where they meet. Locked costs
     * about `cosMid`. General divides by it, turning the entries' absolute rounding error into
     * roughly `machineEpsilon / cosMid`. Those are equal at `sqrt(machineEpsilon)`, which is
     * where these values come from and why they differ per type: about 3.5e-4 for float, whose
     * epsilon is ~1.2e-7, and 1.5e-8 for double at ~2.2e-16.
     *
     * @note Do not tune this by round-tripping random rotations. Uniform angles land near a
     *       pole with probability proportional to the threshold itself, so a random sweep never
     *       samples the case this governs and will happily endorse a value that is orders of
     *       magnitude too small. Sample `pi/2 - delta` directly instead.
     */
    static constexpr T kGimbalEpsilon = std::is_same_v<T, float> ? T(3.5e-4) : T(1.5e-8);

    /*!
     * The euler angles that reproduce this rotation through @ref FromEuler under the same
     * @p order, so `FromEuler(q.ToEuler(o), o)` is `q` up to sign and rounding.
     *
     * Component `n` of the result is always the angle about axis `n` - `[0]` is the rotation
     * about X whichever order asked for it - matching the way `FromEuler` reads its argument.
     *
     * Recovered from the rotation matrix rather than from the components directly. Every order
     * here is a Tait-Bryan triple (three distinct axes), so one extraction serves all six: the
     * middle angle comes out of the single entry that isolates it, and the outer and inner
     * angles come from two entry pairs whose signs carry the quadrant. Which entries those are
     * is fixed by the axis triple and by whether it is an even or odd permutation of (X, Y, Z).
     *
     * @warning Not a bijection, and cannot be. Euler angles are three-to-one onto rotations
     *          (adding π to all three names the same rotation), so the angles that come back
     *          need not be the ones that went in - only the rotation they describe is
     *          preserved. Round-tripping a *value* through this and comparing is a mistake;
     *          round-tripping a rotation is fine.
     *
     * @warning At the poles, where the middle rotation folds the outer and inner axes onto each
     *          other, only their sum (or difference) survives. There the inner angle is pinned
     *          to zero and the whole of it is reported on the outer one, which is a rotation
     *          equal to the original but an angle triple that can look nothing like the input.
     */
    Vec3<T> ToEuler(EulerOrder order = EulerOrder::ZYX) const noexcept {
      /* i, j, k are the axes of the outer, middle and inner rotation, so `FromEuler` built this
       * as Ri * Rj * Rk. `parity` is +1 when (i, j, k) is an even permutation of (0, 1, 2) and
       * -1 when it is odd, which is the only thing that differs between the two halves. */
      size_t i {}, j {}, k {};
      T parity {};

      switch (order) {
      case EulerOrder::XYZ: i = 0; j = 1; k = 2; parity = T(1); break;
      case EulerOrder::XZY: i = 0; j = 2; k = 1; parity = -T(1); break;
      case EulerOrder::YXZ: i = 1; j = 0; k = 2; parity = -T(1); break;
      case EulerOrder::YZX: i = 1; j = 2; k = 0; parity = T(1); break;
      case EulerOrder::ZXY: i = 2; j = 0; k = 1; parity = T(1); break;
      case EulerOrder::ZYX: i = 2; j = 1; k = 0; parity = -T(1); break;
      default: return Vec3<T> {};
      }

      // ToMat4 assumes unit length; a quaternion that has been accumulating products is not.
      const Mat4<T> m = Normalized().ToMat4();
      const auto R = [&m](const size_t _row, const size_t _col) { return m.data[_row * 4 + _col]; };

      Vec3<T> out {};

      const T sinMid = parity * R(i, k); // the one entry the middle angle alone decides

      /* Its cosine comes from the pair of entries that carry it directly, not from
       * sqrt(1 - sin^2): near a pole that subtraction cancels away most of the significant
       * digits, and the angles derived from it inherit the loss. These two square to cos^2
       * exactly, and the middle angle is in [-pi/2, pi/2], so the root is its cosine outright. */
      const T cosMid = Sqrt(R(j, k) * R(j, k) + R(k, k) * R(k, k));

      // Atan2 rather than Asin: same angle, but it stays accurate as cosMid goes to zero.
      out[j] = Atan2(sinMid, cosMid);

      if (cosMid > kGimbalEpsilon) {
        out[i] = Atan2(-parity * R(j, k), R(k, k));
        out[k] = Atan2(-parity * R(i, j), R(i, i));
      } else {
        /* Gimbal lock. The outer and inner axes now coincide, so the matrix only ever saw their
         * sum (or their difference, depending on which pole). Report all of it on the outer
         * angle and none on the inner - any split reproduces the same rotation. */
        out[i] = Atan2(sinMid < T(0) ? -R(j, i) : R(j, i), R(j, j));
        out[k] = T(0);
      }

      return out;
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
