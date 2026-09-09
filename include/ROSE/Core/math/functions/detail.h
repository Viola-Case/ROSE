/**

  @file      detail.h
  @brief
  @details   ~
  @author    Viola Case
  @date      09.09.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/math/constants.h>
#include <ROSE/Core/math/functions/common.h>

/*!
 * @brief Implementation details of the math functions. Not part of the public API.
 *
 * Anything in here exists to serve a function in @ref ROSE::math and may be changed or removed without notice.
 * Nothing outside the `math/functions/` headers should name it directly; call the wrapper instead, which handles the
 * dispatch and the edge cases the helpers assume have already been ruled out.
 *
 * This header holds what more than one of those functions needs for its constant-evaluated path: raw access to the
 * bit pattern of a double, exact scaling by a power of two, truncation, and a small double-double arithmetic kit.
 * The double-double kit is what lets @ref Ln, @ref Exp and @ref Pow fold to the correctly rounded result rather than
 * to "within an ulp": every intermediate carries ~106 bits, so the single rounding that matters is the last one.
 */
namespace ROSE::math::detail {
#pragma region bits
  constexpr uint64_t Bits(double _value) noexcept { return __builtin_bit_cast(uint64_t, _value); }
  constexpr uint32_t Bits(float _value) noexcept { return __builtin_bit_cast(uint32_t, _value); }
  constexpr double FromBits(uint64_t _bits) noexcept { return __builtin_bit_cast(double, _bits); }
  constexpr float FromBits(uint32_t _bits) noexcept { return __builtin_bit_cast(float, _bits); }

  constexpr int      MANTISSABITS64 = 52;   //!< explicit mantissa bits of a double
  constexpr int      EXPONENTBIAS64 = 1023; //!< exponent bias of a double
  constexpr uint64_t MANTISSAMASK64 = (uint64_t { 1 } << MANTISSABITS64) - 1;
  constexpr uint64_t IMPLICITBIT64 = uint64_t { 1 } << MANTISSABITS64; //!< the hidden leading 1 of a normal double
  constexpr int      MANTISSABITS32 = 23;                              //!< explicit mantissa bits of a float
  constexpr int      EXPONENTBIAS32 = 127;                             //!< exponent bias of a float
  constexpr uint32_t MANTISSAMASK32 = (uint32_t { 1 } << MANTISSABITS32) - 1;
  constexpr uint32_t IMPLICITBIT32 = uint32_t { 1 } << MANTISSABITS32;

  /*! Biased exponent field of a double: 0 for zero and subnormals, 2047 for ±∞ and NaN. */
  constexpr int ExponentField(double _value) noexcept {
    return static_cast<int>((Bits(_value) >> MANTISSABITS64) & 0x7FF);
  }
  constexpr int ExponentField(float _value) noexcept {
    return static_cast<int>((Bits(_value) >> MANTISSABITS32) & 0xFF);
  }

  /*! \f$2^k\f$ as a double, for k in [-1022, 1023]. Built straight from the exponent field, so it is exact and free. */
  constexpr double Pow2(int _k) noexcept {
    return FromBits(static_cast<uint64_t>(_k + EXPONENTBIAS64) << MANTISSABITS64);
  }

  /*!
   * \f$v \cdot 2^k\f$ for any `k`, rounding at most once - the constant-evaluated stand-in for `ldexp`, which Clang
   * will not fold.
   *
   * A single multiply by \f$2^k\f$ only works while \f$2^k\f$ is itself a double, so large `k` is applied in up to
   * three steps. On the way down the intermediate steps deliberately stop at least 53 bits short of the subnormal
   * range: a value that has already been rounded into a subnormal and is then scaled again would round twice, and
   * the second rounding can land on the wrong neighbour. Stopping short means only the final multiply can round.
   */
  constexpr double Scale(double _value, int _k) noexcept {
    if (_k > 1023) {
      _value *= 0x1p1023;
      _k -= 1023;
      if (_k > 1023) {
        _value *= 0x1p1023;
        _k -= 1023;
        if (_k > 1023) _k = 1023; // overflow either way; the final multiply produces the ∞
      }
    } else if (_k < -1022) {
      _value *= 0x1p-1022 * 0x1p53;
      _k += 1022 - 53;
      if (_k < -1022) {
        _value *= 0x1p-1022 * 0x1p53;
        _k += 1022 - 53;
        if (_k < -1022) _k = -1022; // underflow either way; the final multiply produces the 0
      }
    }
    return _value * Pow2(_k);
  }

  /*!
   * A 128-bit unsigned integer correctly rounded to a double. `static_cast<double>` on a `uint128_t` would do the
   * same, but it lowers to compiler-rt's `__floatuntidf`, which the MSVC-target link does not have; keeping the top
   * 53 bits and rounding on the guard and sticky bits by hand is a handful of shifts instead.
   */
  constexpr double FromUInt128(uint128_t _value) noexcept {
    if (_value == 0) return 0.0;
    int msb = 127;
    while (((_value >> msb) & 1) == 0)
      --msb;
    if (msb < 53) return static_cast<double>(static_cast<uint64_t>(_value)); // exact
    const int       shift = msb - 52;
    uint64_t        q = static_cast<uint64_t>(_value >> shift);
    const uint128_t rem = _value & ((uint128_t { 1 } << shift) - 1);
    const uint128_t half = uint128_t { 1 } << (shift - 1);
    if (rem > half || (rem == half && (q & 1))) ++q; // nearest, ties to even; q may reach 2^53, still exact
    return Scale(static_cast<double>(q), shift);
  }
#pragma endregion

#pragma region rounding
  /*!
   * Round toward zero. `__builtin_trunc` does not fold, so: at or above \f$2^{52}\f$ every double is already an
   * integer (and ±∞ and NaN fail the comparison and pass through as well); below it a round trip through `int64_t`
   * truncates. The sign is copied back so that `Trunc(-0.5)` is `-0.0`, as `std::trunc` gives.
   */
  constexpr double Trunc(double _value) noexcept {
    if (!(Abs(_value) < 0x1p52)) return _value;
    const double t = static_cast<double>(static_cast<int64_t>(_value));
    return t == 0.0 ? __builtin_copysign(0.0, _value) : t;
  }

  /*! Round half away from zero, for turning a quotient into an integer step count. */
  constexpr double RoundHalfAway(double _value) noexcept { return Trunc(_value + __builtin_copysign(0.5, _value)); }

  /*! `true` for a finite value with no fractional part. ±∞ are not integers here, whatever `std::pow` thinks. */
  constexpr bool IsInteger(double _value) noexcept { return IsFinite(_value) && Trunc(_value) == _value; }

  /*! `true` for an odd integer. Every double at or above \f$2^{53}\f$ is an even integer, so those short-circuit. */
  constexpr bool IsOddInteger(double _value) noexcept {
    if (!IsInteger(_value) || !(Abs(_value) < 0x1p53)) return false;
    const double half = _value * 0.5; // exact
    return Trunc(half) != half;
  }
#pragma endregion

#pragma region double-double
  /*!
   * An unevaluated sum `hi + lo` with \f$|lo| \le \frac{1}{2}\,\text{ulp}(hi)\f$, carrying ~106 bits of precision in
   * two doubles.
   *
   * The operations below are the classical error-free transformations (Knuth's TwoSum, Dekker's split and
   * TwoProduct) and the double-double arithmetic built from them - see Hida, Li & Bailey, *Library for Double-Double
   * and Quad-Double Arithmetic*. Nothing here is fast, and nothing here needs to be: it only ever runs inside a
   * constant expression, where what matters is that the result rounds to the right double.
   *
   * @warning Dekker's split multiplies by \f$2^{27}+1\f$ and overflows above roughly \f$2^{996}\f$. Every caller in
   *          this directory keeps its operands well below that; a new one must too.
   */
  struct DD {
    double hi;
    double lo;
  };

  /*! `a + b` exactly, as a `DD`. No precondition. */
  constexpr DD TwoSum(double _a, double _b) noexcept {
    const double s = _a + _b;
    const double bb = s - _a;
    return { s, (_a - (s - bb)) + (_b - bb) };
  }

  /*! `a + b` exactly, as a `DD`, for \f$|a| \ge |b|\f$ (or `a == 0`). Three flops instead of six. */
  constexpr DD FastTwoSum(double _a, double _b) noexcept {
    const double s = _a + _b;
    return { s, _b - (s - _a) };
  }

  /*! `a * b` exactly, as a `DD`. Dekker's algorithm, since `__builtin_fma` does not fold. */
  constexpr DD TwoProd(double _a, double _b) noexcept {
    constexpr double SPLITTER = 134217729.0; // 2^27 + 1
    const double     p = _a * _b;
    const double     ta = SPLITTER * _a;
    const double     ahi = ta - (ta - _a);
    const double     alo = _a - ahi;
    const double     tb = SPLITTER * _b;
    const double     bhi = tb - (tb - _b);
    const double     blo = _b - bhi;
    return { p, ((ahi * bhi - p) + ahi * blo + alo * bhi) + alo * blo };
  }

  constexpr DD Add(DD _a, DD _b) noexcept {
    DD       s = TwoSum(_a.hi, _b.hi);
    const DD t = TwoSum(_a.lo, _b.lo);
    s.lo += t.hi;
    s = FastTwoSum(s.hi, s.lo);
    s.lo += t.lo;
    return FastTwoSum(s.hi, s.lo);
  }
  constexpr DD Sub(DD _a, DD _b) noexcept { return Add(_a, { -_b.hi, -_b.lo }); }
  constexpr DD Mul(DD _a, DD _b) noexcept {
    DD p = TwoProd(_a.hi, _b.hi);
    p.lo += _a.hi * _b.lo + _a.lo * _b.hi;
    return FastTwoSum(p.hi, p.lo);
  }
  constexpr DD Mul(DD _a, double _b) noexcept {
    DD p = TwoProd(_a.hi, _b);
    p.lo += _a.lo * _b;
    return FastTwoSum(p.hi, p.lo);
  }
  /*! Two rounds of long division: quotient, exact remainder, correction. Accurate to ~\f$2^{-104}\f$. */
  constexpr DD Div(DD _a, double _b) noexcept {
    const double q1 = _a.hi / _b;
    const DD     r = Sub(_a, TwoProd(q1, _b));
    const double q2 = r.hi / _b;
    return FastTwoSum(q1, q2);
  }
  constexpr DD Div(DD _a, DD _b) noexcept {
    const double q1 = _a.hi / _b.hi;
    DD           r = Sub(_a, Mul(_b, q1));
    const double q2 = r.hi / _b.hi;
    r = Sub(r, Mul(_b, q2));
    const double q3 = r.hi / _b.hi;
    return Add(FastTwoSum(q1, q2), { q3, 0.0 });
  }

  /*! The `DD` collapsed to the nearest double. A normalised `DD` already has `hi` there, but say so. */
  constexpr double Collapse(DD _value) noexcept { return _value.hi + _value.lo; }

  /*!
   * Constants to ~106 bits: the double nearest the value and the double nearest the remainder. The `DD` suffix
   * keeps them apart from the plain-double `math::PI`, which is what the engine wants everywhere else.
   */
  constexpr DD LN2DD = { 6.93147180559945286227e-01, 2.31904681384629955842e-17 };   //!< ln 2
  constexpr DD LN10DD = { 2.30258509299404590109e+00, -2.17075622338224935110e-16 }; //!< ln 10
  constexpr DD PIDD = { 3.14159265358979311600e+00, 1.22464679914735320717e-16 };    //!< π
  constexpr DD PIO2DD = { 1.57079632679489655800e+00, 6.12323399573676603587e-17 };  //!< π/2
#pragma endregion
} // namespace ROSE::math::detail
