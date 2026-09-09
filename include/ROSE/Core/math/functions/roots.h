/**

  @file      roots.h
  @brief
  @details   ~
  @author    Viola Case
  @date      09.09.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/math/functions/detail.h>

namespace ROSE::math {
  namespace detail {
    /*!
     * \f$\lfloor\sqrt{n}\rfloor\f$ and the remainder \f$n - \lfloor\sqrt{n}\rfloor^2\f$, by the binary digit-by-digit
     * method. One iteration per result bit, every step exact; only ever runs in a constant expression.
     *
     * `U` is any unsigned integer type, including `uint128_t`, which `std::unsigned_integral` does not admit in
     * strict `-std=c++20` mode - hence the bare `typename`.
     */
    template <typename U>
    constexpr U ISqrt(U _n, U &_remainder) noexcept {
      U result { 0 };
      U bit = U { 1 } << (sizeof(U) * 8 - 2); // the largest power of four that fits
      while (bit > _n)
        bit >>= 2;
      while (bit != 0) {
        if (_n >= result + bit) {
          _n -= result + bit;
          result = (result >> 1) + bit;
        } else {
          result >>= 1;
        }
        bit >>= 2;
      }
      _remainder = _n;
      return result;
    }

    /*!
     * Correctly rounded square root, for the constant-evaluated path only.
     *
     * Rather than iterate toward the root in floating point and hope the last bit lands, this takes the root of the
     * mantissa as an *integer*. Write \f$v = m \cdot 2^E\f$ with \f$m\f$ the 53-bit integer mantissa; shift once if
     * needed so that \f$E\f$ is even, which puts \f$m\f$ in \f$[2^{52}, 2^{54})\f$ and makes \f$\sqrt{2^E}\f$ an
     * exact power of two. Then \f$\lfloor\sqrt{m \cdot 2^{56}}\rfloor\f$ is a 55-bit integer whose top 53 bits are
     * the result mantissa and whose bottom two, together with whether the remainder was zero, are exactly the guard
     * and sticky bits IEEE rounding wants. The 110-bit radicand needs `uint128_t`, which every compiler that gets
     * this far provides.
     *
     * The result is therefore bit-identical to the hardware `sqrtsd`, and a folded @ref Sqrt can be compared
     * against a runtime one, or `static_assert`ed against a literal, without a caveat.
     */
    constexpr double SqrtConst(double _value) noexcept {
      if (!(_value > 0.0)) return (_value < 0.0 ? __builtin_nan("") : _value); // ±0, NaN, negatives
      if (_value > MAXFINITE64) return _value;                                 // +∞

      const uint64_t bits = Bits(_value);
      const int      field = ExponentField(_value);
      uint64_t       m = bits & MANTISSAMASK64;
      int            e = -EXPONENTBIAS64 - MANTISSABITS64; // v = m · 2^(e + field)

      if (field == 0) { // subnormal: no implicit bit, so normalise by hand
        e += 1;
        while (!(m & IMPLICITBIT64)) {
          m <<= 1;
          --e;
        }
      } else {
        m |= IMPLICITBIT64;
        e += field;
      }
      if (e & 1) { // make the exponent even so its root is a power of two
        m <<= 1;
        --e;
      }

      uint128_t       remainder = 0;
      const uint128_t s = ISqrt(static_cast<uint128_t>(m) << 56, remainder); // ∈ [2^54, 2^55)
      uint64_t        q = static_cast<uint64_t>(s >> 2);                     // 53 bits
      const bool      guard = (s >> 1) & 1;
      const bool      sticky = (s & 1) || remainder != 0;
      if (guard && (sticky || (q & 1))) ++q; // round to nearest, ties to even
      return Scale(static_cast<double>(q), e / 2 - 26);
    }

    /*!
     * A float root computed as a double root and narrowed. That is not merely "close": rounding to 53 bits and then
     * to 24 gives the correctly rounded 24-bit result whenever the wide format has at least \f$2p + 2\f$ bits, which
     * 53 comfortably is for \f$p = 24\f$ (Figueroa, 1995). So this too matches `sqrtss` bit for bit.
     */
    constexpr float SqrtConst(float _value) noexcept {
      return static_cast<float>(SqrtConst(static_cast<double>(_value)));
    }

    /*!
     * Square root of a double-double, as a double-double: the correctly rounded root of the head, then one Newton
     * step on the exact residual - \f$\sqrt{h + l} \approx r + (h - r^2 + l) / 2r\f$ with \f$h - r^2\f$ taken
     * exactly - recovers the tail. `a` must be non-negative; a zero head returns zero.
     */
    constexpr DD SqrtDD(DD _a) noexcept {
      const double r = SqrtConst(_a.hi);
      if (r == 0.0) return { 0.0, 0.0 };
      const DD residual = Add(Sub({ _a.hi, 0.0 }, TwoProd(r, r)), { _a.lo, 0.0 });
      return FastTwoSum(r, Collapse(residual) / (2.0 * r));
    }

    /*!
     * \f$\sqrt{x^2 + y^2}\f$ without spurious overflow or underflow, for the constant-evaluated path only.
     *
     * The operands are scaled by a power of two into a range where the squares are safe, the sum of squares is formed
     * exactly as a double-double, and the root of that is the correctly rounded root of its high part plus one Newton
     * correction for the low part. Net error is well under half an ulp before the final rounding, so the result is the
     * correctly rounded one in all but the rare hard cases.
     */
    constexpr double HypotConst(double _x, double _y) noexcept {
      double a = Abs(_x);
      double b = Abs(_y);
      if (a < b) {
        const double t = a;
        a = b;
        b = t;
      }
      if (IsInf(a) || IsInf(b)) return __builtin_inf(); // ∞ wins even against NaN, per IEEE 754
      if (IsNaN(a)) return a; // not `a + b`: Clang refuses arithmetic that yields a NaN in a constant expression
      if (IsNaN(b)) return b;
      if (b == 0.0) return a;

      int scale = 0;
      if (a > 0x1p500) {
        scale = 600;
        a = Scale(a, -600);
        b = Scale(b, -600);
      } else if (a < 0x1p-500) {
        scale = -600;
        a = Scale(a, 600);
        b = Scale(b, 600);
      }
      return Scale(Collapse(SqrtDD(Add(TwoProd(a, a), TwoProd(b, b)))), scale);
    }

    /*!
     * A float hypot has no overflow problem in double - the largest float squared is ~\f$10^{77}\f$ - so it is just the
     * naive formula in the wider type, which is accurate to under an ulp of float. The runtime path does the same.
     */
    constexpr float HypotConst(float _x, float _y) noexcept {
      const double dx = _x;
      const double dy = _y;
      if (IsInf(dx) || IsInf(dy)) return __builtin_inff();
      return static_cast<float>(SqrtConst(dx * dx + dy * dy));
    }
  } // namespace detail

  /*!
   * Square root usable in a constant expression.
   *
   * `std::sqrt` is not constexpr until C++26 and Clang will not fold `__builtin_sqrt` either, so constant evaluation
   * goes through @ref detail::SqrtConst, which takes the root of the mantissa as an integer and rounds it exactly. At
   * runtime the builtin lowers to a single hardware `sqrt`, which is correctly rounded by definition. The two paths
   * agree bit for bit, in both precisions, including for ±0, subnormals, ∞ and NaN.
   */
  constexpr double Sqrt(double _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::SqrtConst(_value);
    return __builtin_sqrt(_value);
  }
  constexpr float Sqrt(float _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::SqrtConst(_value);
    return __builtin_sqrtf(_value);
  }

  /*!
   * \f$\sqrt{x^2 + y^2}\f$, without the intermediate overflow or underflow the naive formula suffers.
   *
   * The double version defers to libm at runtime - scaling the operands and taking one root is exactly what libm does,
   * and there is nothing faster that is also safe. The float version does *not*: a float's square cannot overflow a
   * double, so it squares and roots in double and narrows, which is one hardware `sqrt` and accurate to under an ulp,
   * against a libm call that would have to scale. Both fold in a constant expression, via @ref detail::HypotConst.
   *
   * Mixed and integral arguments promote to double.
   */
  constexpr double Hypot(double _x, double _y) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::HypotConst(_x, _y);
    return __builtin_hypot(_x, _y);
  }
  constexpr float Hypot(float _x, float _y) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::HypotConst(_x, _y);
    const double dx = _x;
    const double dy = _y;
    if (IsInf(dx) || IsInf(dy)) return __builtin_inff();
    return static_cast<float>(__builtin_sqrt(dx * dx + dy * dy));
  }
  template <StdScalar A, StdScalar B>
  constexpr double Hypot(A _x, B _y) noexcept {
    return Hypot(static_cast<double>(_x), static_cast<double>(_y));
  }
} // namespace ROSE::math
