/**

  @file      exponential.h
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
     * \f$\ln v\f$ as a double-double, for the constant-evaluated path only. `v` must be positive and finite.
     *
     * Write \f$v = m \cdot 2^k\f$ with \f$m \in [\sqrt{1/2}, \sqrt{2})\f$, so that \f$\ln v = k \ln 2 + \ln m\f$
     * and \f$f = m - 1\f$ is exact (Sterbenz). Then \f$\ln m = 2\,\text{artanh}(s)\f$ with \f$s = f / (2 + f)\f$,
     * whose series \f$2s(1 + s^2/3 + s^4/5 + \dots)\f$ has \f$|s| < 0.172\f$ and so converges by a factor of ~34
     * per term: twenty-two terms reach \f$2^{-106}\f$. Every step is in double-double, and \f$\ln 2\f$ is
     * @ref detail::LN2, also to 106 bits, so the result is the true logarithm to ~100 bits and collapses to the
     * correctly rounded double.
     */
    constexpr DD LnDD(double _value) noexcept {
      int k = 0;
      if (_value < MINNORMAL64) { // subnormal: bring it into range first; the scale is exact
        _value *= 0x1p54;
        k -= 54;
      }
      k += ExponentField(_value) - EXPONENTBIAS64;
      double m =
        FromBits((Bits(_value) & MANTISSAMASK64) | (uint64_t { EXPONENTBIAS64 } << MANTISSABITS64)); // ∈ [1, 2)
      if (m > SQRT2) {
        m *= 0.5;
        ++k;
      }

      const double f = m - 1.0;
      const DD     s = Div({ f, 0.0 }, TwoSum(2.0, f));
      const DD     z = Mul(s, s);
      DD           sum { 1.0, 0.0 };
      DD           term { 1.0, 0.0 };
      for (int n = 3; n <= 45; n += 2) {
        term = Mul(term, z);
        sum = Add(sum, Div(term, static_cast<double>(n)));
      }
      const DD lnm = Mul(s, { 2.0 * sum.hi, 2.0 * sum.lo });
      return Add(Mul(LN2DD, static_cast<double>(k)), lnm);
    }

    /*!
     * \f$e^x\f$ for a double-double `x`, correctly rounded to a double, for the constant-evaluated path only. `x.hi`
     * must be within the range where the result is representable or subnormal; the callers check.
     *
     * Reduce by the nearest multiple of \f$\ln 2\f$ - \f$x = k \ln 2 + r\f$, \f$|r| \le \ln 2 / 2\f$ - then sum
     * the Taylor series for \f$e^r\f$ in double-double. On that range twenty-four terms are past \f$2^{-106}\f$.
     *
     * The final \f$2^k\f$ goes through @ref Scale. For a normal result that is exact, so collapsing the
     * double-double first is the only rounding. For a subnormal result the scale rounds *again*, and two roundings
     * can land on the wrong neighbour; so there the collapse is done round-to-odd - the low bit of the collapsed
     * value is forced set whenever the tail was nonzero - which is the classic way to make a second rounding
     * behave as if it were the only one.
     */
    constexpr double ExpCore(DD _x) noexcept {
      constexpr double INVLN2 = 1.44269504088896338700e+00;
      const double     k = RoundHalfAway(_x.hi * INVLN2);
      const DD         r = Sub(_x, Mul(LN2DD, k));

      DD sum = Add({ 1.0, 0.0 }, r);
      DD term = r;
      for (int n = 2; n <= 24; ++n) {
        term = Div(Mul(term, r), static_cast<double>(n));
        sum = Add(sum, term);
      }
      const int ki = static_cast<int>(k);
      if (ki >= -1021) return Scale(Collapse(sum), ki);

      double s = Collapse(sum); // == sum.hi; the tail is what rounding threw away
      if (sum.lo != 0.0 && (Bits(s) & 1) == 0) s = FromBits(Bits(s) + (sum.lo > 0.0 ? 1 : -1));
      return Scale(s, ki);
    }

    constexpr double EXPOVERFLOW = 709.79;  //!< above this, e^x is past MAXFINITE64 (the edge is 709.7827...)
    constexpr double EXPUNDERFLOW = -745.2; //!< below this, e^x rounds to zero (the edge is -745.1332...)

    constexpr double ExpConst(double _value) noexcept {
      if (IsNaN(_value)) return _value;
      if (_value > EXPOVERFLOW) return __builtin_inf();
      if (_value < EXPUNDERFLOW) return 0.0;
      return ExpCore({ _value, 0.0 });
    }
    constexpr double LnConst(double _value) noexcept {
      if (IsNaN(_value)) return _value;
      if (_value < 0.0) return __builtin_nan("");
      if (_value == 0.0) return -__builtin_inf();
      if (IsInf(_value)) return _value;
      return Collapse(LnDD(_value));
    }

    /*!
     * \f$\log_2\f$ and \f$\log_{10}\f$ are the natural logarithm divided, in double-double, by @ref LN2DD and
     * @ref LN10DD. Dividing rather than multiplying by a stored reciprocal costs nothing here and keeps an exact
     * power of the base exact: \f$\ln 2^k\f$ comes out of @ref LnDD as \f$k \cdot \text{LN2DD}\f$ to the last bit,
     * and dividing that by the same constant is \f$k\f$ to within \f$2^{-105}\f$, which collapses to \f$k\f$.
     */
    constexpr double Log2Const(double _value) noexcept {
      if (IsNaN(_value)) return _value;
      if (_value < 0.0) return __builtin_nan("");
      if (_value == 0.0) return -__builtin_inf();
      if (IsInf(_value)) return _value;
      return Collapse(Div(LnDD(_value), LN2DD));
    }
    constexpr double Log10Const(double _value) noexcept {
      if (IsNaN(_value)) return _value;
      if (_value < 0.0) return __builtin_nan("");
      if (_value == 0.0) return -__builtin_inf();
      if (IsInf(_value)) return _value;
      return Collapse(Div(LnDD(_value), LN10DD));
    }

    /*!
     * \f$x^y\f$, for the constant-evaluated path only.
     *
     * The special cases are the ones IEEE 754 and C's Annex F prescribe, in their order: anything to the zeroth
     * power and one to any power are 1, even for a NaN; then NaN propagates; then zero, infinite and negative bases
     * each get their sign and their integer-exponent rules. What remains is \f$e^{y \ln|x|}\f$ with the logarithm
     * and the product both in double-double, which is what keeps the result correctly rounded when \f$y \ln|x|\f$
     * is large and the usual double-precision product would have thrown away the bits the exponential then
     * amplifies. Exponents beyond \f$2^{64}\f$ in magnitude short-circuit to the overflow or underflow they
     * necessarily produce, which also keeps the double-double product clear of Dekker's split overflowing.
     */
    constexpr double PowConst(double _x, double _y) noexcept {
      if (_y == 0.0 || _x == 1.0) return 1.0;
      if (IsNaN(_x)) return _x; // not `_x + _y`: Clang refuses arithmetic that yields a NaN in a constant expression
      if (IsNaN(_y)) return _y;

      const bool yIsInteger = IsInteger(_y);
      const bool yIsOdd = IsOddInteger(_y);
      if (_x == 0.0) {
        const bool negative = __builtin_signbit(_x) && yIsOdd; // only an odd exponent keeps -0's sign
        if (_y < 0.0) return negative ? -__builtin_inf() : __builtin_inf();
        return negative ? -0.0 : 0.0;
      }
      if (IsInf(_y)) {
        if (_x == -1.0) return 1.0;
        return ((Abs(_x) < 1.0) == (_y < 0.0)) ? __builtin_inf() : 0.0;
      }
      if (IsInf(_x)) {
        if (_x < 0.0) {
          if (_y < 0.0) return yIsOdd ? -0.0 : 0.0;
          return yIsOdd ? -__builtin_inf() : __builtin_inf();
        }
        return _y < 0.0 ? 0.0 : __builtin_inf();
      }
      if (_x < 0.0 && !yIsInteger) return __builtin_nan("");

      const double ax = Abs(_x);
      const double sign = (_x < 0.0 && yIsOdd) ? -1.0 : 1.0;

      if (yIsInteger && _y > 0.0) {
        /* Exact path. Write |x| = m · 2^e with m an odd integer. A power of two is then exact for any exponent, and
         * for a positive integer exponent n small enough that m^n fits in 128 bits, |x|^n = m^n · 2^(en) computed as
         * an integer and converted once is the correctly rounded result even when the true value is an exact tie
         * between two doubles - 10^23 is 5^23 · 2^23 and is one - which the ~106 bits of the double-double path
         * below cannot decide. */
        uint64_t m = Bits(ax) & MANTISSAMASK64;
        int      e = ExponentField(ax);
        if (e == 0) e = 1;
        else m |= IMPLICITBIT64;
        e -= EXPONENTBIAS64 + MANTISSABITS64;
        while ((m & 1) == 0) {
          m >>= 1;
          ++e;
        }
        if (m == 1) {
          const double shift = e * _y; // exact, or so far out of range that rounding it cannot matter
          if (shift > 1024.0) return sign * __builtin_inf();
          if (shift < -1075.0) return sign * 0.0;
          return sign * Scale(1.0, static_cast<int>(shift));
        }
        if (_y <= 128.0) {
          const int n = static_cast<int>(_y);
          int       mBits = 0;
          while ((m >> mBits) != 0)
            ++mBits;
          uint128_t p = 1;
          bool      fits = true;
          for (int i = 0; i < n; ++i) {
            /* p·m < 2^128 whenever p has room for m's bits above it. No 128-bit division: that is a compiler-rt
             * libcall (`__udivti3`) the MSVC-target link cannot resolve, and unlike the shifts and multiplies it
             * would survive into an unoptimised build even though only constant evaluation ever reaches it. */
            if ((p >> (128 - mBits)) != 0) {
              fits = false;
              break;
            }
            p *= m;
          }
          /* Below 2^-1021 the scale could land in the subnormal range and round a second time; leave that to the
           * double-double path, which knows how to round once there. Overflow needs no guard: the integer rounds
           * exactly as the real result would, and the scale then overflows exactly when that rounded result does. */
          if (fits && e * n >= -1021) return sign * Scale(FromUInt128(p), e * n);
        }
      }

      if (Abs(_y) > 0x1p64) {
        /* |ln ax| is at least ~2^-53 for any ax ≠ 1, so |y ln ax| is past 2^11 and the result is certainly out of
         * range; ax == 1 can only be x == -1, and an exponent this large is even. */
        if (ax == 1.0) return 1.0;
        return ((ax > 1.0) == (_y > 0.0)) ? sign * __builtin_inf() : sign * 0.0;
      }

      const DD p = Mul(LnDD(ax), _y);
      if (p.hi > EXPOVERFLOW) return sign * __builtin_inf();
      if (p.hi < EXPUNDERFLOW) return sign * 0.0;
      return sign * ExpCore(p);
    }

    /*!
     * The float versions compute in double and narrow. That is the accurate choice, not a shortcut: the double
     * result is within a fraction of a double ulp, which is \f$2^{29}\f$ times finer than a float ulp, so the
     * narrowed result is the correctly rounded float except when the true value sits almost exactly on a float
     * midpoint.
     */
    constexpr float ExpConst(float _value) noexcept {
      return static_cast<float>(ExpConst(static_cast<double>(_value)));
    }
    constexpr float LnConst(float _value) noexcept { return static_cast<float>(LnConst(static_cast<double>(_value))); }
    constexpr float Log2Const(float _value) noexcept {
      return static_cast<float>(Log2Const(static_cast<double>(_value)));
    }
    constexpr float Log10Const(float _value) noexcept {
      return static_cast<float>(Log10Const(static_cast<double>(_value)));
    }
    constexpr float PowConst(float _x, float _y) noexcept {
      return static_cast<float>(PowConst(static_cast<double>(_x), static_cast<double>(_y)));
    }

    /*!
     * Exponentiation by squaring, for the runtime integer-exponent @ref Pow.
     *
     * Each squaring doubles the relative error already accumulated, so the worst case grows like \f$|n|\f$ ulps,
     * not \f$\log_2|n|\f$: measured against libm, exponents up to ±8 stay within a handful of ulps and ±40 reaches
     * ~30. @ref Pow only routes exponents within @ref POWINTLIMIT here and sends the rest to libm.
     */
    constexpr int POWINTLIMIT = 8;
    template <std::floating_point T>
    constexpr T PowInt(T _base, int _exponent) noexcept {
      unsigned n = _exponent < 0 ? 0u - static_cast<unsigned>(_exponent) : static_cast<unsigned>(_exponent);
      T        result { 1 };
      while (n != 0) {
        if (n & 1) result *= _base;
        n >>= 1;
        if (n != 0) _base *= _base;
      }
      return _exponent < 0 ? T { 1 } / result : result;
    }
  } // namespace detail

  /*!
   * \f$e^x\f$, usable in a constant expression.
   *
   * At runtime this is `__builtin_exp`, i.e. libm. At compile time it is @ref detail::ExpConst, which sums the series
   * in double-double and so folds to the correctly rounded value; the two agree wherever libm is itself correctly
   * rounded, which for `exp` is nearly everywhere.
   */
  constexpr double Exp(double _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::ExpConst(_value);
    return __builtin_exp(_value);
  }
  constexpr float Exp(float _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::ExpConst(_value);
    return __builtin_expf(_value);
  }

  /*!
   * Natural logarithm, usable in a constant expression.
   *
   * At runtime this is `__builtin_log`, i.e. libm. At compile time it is @ref detail::LnConst, which evaluates the
   * artanh series in double-double and so folds to the correctly rounded value. `Ln(1)` is exactly `+0`, `Ln(0)` is
   * `-∞`, a negative argument is NaN.
   */
  constexpr double Ln(double _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::LnConst(_value);
    return __builtin_log(_value);
  }
  constexpr float Ln(float _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::LnConst(_value);
    return __builtin_logf(_value);
  }

  /*!
   * Base-2 and base-10 logarithms, usable in a constant expression. libm at runtime; at compile time the
   * double-double natural logarithm divided by the double-double constant, which is correctly rounded and exact
   * for exact powers of the base - `Log2(1024.0) == 10.0` and `Log10(1e22) == 22.0` fold as written.
   */
  constexpr double Log2(double _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::Log2Const(_value);
    return __builtin_log2(_value);
  }
  constexpr float Log2(float _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::Log2Const(_value);
    return __builtin_log2f(_value);
  }
  constexpr double Log10(double _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::Log10Const(_value);
    return __builtin_log10(_value);
  }
  constexpr float Log10(float _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::Log10Const(_value);
    return __builtin_log10f(_value);
  }

  /*!
   * \f$x^y\f$, usable in a constant expression.
   *
   * At runtime the floating-point-exponent overloads are `__builtin_pow`, i.e. libm, which is within an ulp and
   * handles every IEEE special case. At compile time they go through @ref detail::PowConst, which is correctly
   * rounded - exactly, via integer arithmetic, for a positive integer exponent on a base with a small odd mantissa,
   * and through ~106-bit intermediates otherwise - and follows the same special-case table, so
   * `Pow(-8.0, 1.0 / 3.0)` is NaN and `Pow(-2.0, 3.0)` is `-8` on both paths.
   *
   * The `int`-exponent overloads are the fast path for the common "square it, cube it" call: at runtime an exponent
   * of magnitude up to @ref detail::POWINTLIMIT is exponentiation by squaring - a couple of multiplies against a
   * libm call, at the cost of a few ulps of rounding rather than one - and anything larger goes to libm, where the
   * squaring's error would have grown past what a libm call costs. At compile time they fold through the same
   * correctly rounded path as the others, so a folded `Pow(x, 3)` is the true cube rounded once. Prefer these
   * whenever the exponent is a literal integer.
   *
   * Mixed and integral arguments promote: an integral exponent that fits in an `int` takes the fast path, everything
   * else is computed in double.
   */
  constexpr double Pow(double _base, double _exponent) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::PowConst(_base, _exponent);
    return __builtin_pow(_base, _exponent);
  }
  constexpr float Pow(float _base, float _exponent) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::PowConst(_base, _exponent);
    return __builtin_powf(_base, _exponent);
  }
  constexpr double Pow(double _base, int _exponent) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::PowConst(_base, static_cast<double>(_exponent));
    if (_exponent >= -detail::POWINTLIMIT && _exponent <= detail::POWINTLIMIT) return detail::PowInt(_base, _exponent);
    return __builtin_pow(_base, static_cast<double>(_exponent));
  }
  constexpr float Pow(float _base, int _exponent) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::PowConst(_base, static_cast<float>(_exponent));
    if (_exponent >= -detail::POWINTLIMIT && _exponent <= detail::POWINTLIMIT) return detail::PowInt(_base, _exponent);
    return __builtin_powf(_base, static_cast<float>(_exponent));
  }
  template <StdScalar A, StdScalar B>
  constexpr double Pow(A _base, B _exponent) noexcept {
    if constexpr (std::integral<B> && sizeof(B) <= sizeof(int))
      return Pow(static_cast<double>(_base), static_cast<int>(_exponent));
    else return Pow(static_cast<double>(_base), static_cast<double>(_exponent));
  }
} // namespace ROSE::math
