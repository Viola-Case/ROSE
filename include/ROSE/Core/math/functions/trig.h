/**

  @file      trig.h
  @brief
  @details   ~
  @author    Viola Case
  @date      09.09.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/math/functions/detail.h>
#include <ROSE/Core/math/functions/roots.h>

namespace ROSE::math {
  namespace detail {
    /*!
     * Sine and cosine together, for the constant-evaluated path only.
     *
     * `std::sin`/`std::cos` are not constexpr until C++26 and Clang will not fold `__builtin_sin` in a constant
     * expression either, so compile-time evaluation reduces the argument into \f$[-\pi/4, \pi/4]\f$ and sums a
     * Taylor series there.
     *
     * The reduction subtracts \f$k \cdot \pi/2\f$ in double-double arithmetic (see @ref detail::DD), with
     * \f$\pi/2\f$ carried to 159 bits, so the reduced argument comes out as a head and a tail that together hold
     * ~106 bits. That keeps the result accurate up to roughly \f$|v| < 2^{50}\f$, well past the \f$2^{20}\f$ a
     * three-part Cody–Waite split manages, and the tail feeds the polynomials as \f$y\cos x\f$ and \f$-y\sin x\f$
     * corrections in the fdlibm manner. The step count \f$k\f$ is kept as a double and reduced modulo 4 in floating
     * point, which is exact for an integer-valued double of any size; nothing here casts to an integer that could
     * overflow. The engine's @ref ROSE::math::PI is a single double and would poison the reduction, so the split
     * constants live here.
     *
     * The series run to \f$x^{17}\f$ for sine and \f$x^{16}\f$ for cosine, whose next terms are below
     * \f$10^{-19}\f$ on the reduced range - past double precision - and are arranged so that the exact leading term
     * is disturbed by only one rounding. Measured against a correctly rounded reference the result is within 1 ulp;
     * a double-double evaluation would be needed to go further, and the runtime builtins are the accurate path for
     * astronomically large arguments.
     *
     * Both are returned at once because the quadrant dispatch computes them from the same reduced polynomial;
     * @ref Sin, @ref Cos and @ref Tan all read from this.
     */
    struct SinCosPair {
      double sin;
      double cos;
    };
    constexpr SinCosPair SinCosConst(double _value) noexcept {
      if (IsNaN(_value)) return { _value, _value };
      if (IsInf(_value)) return { __builtin_nan(""), __builtin_nan("") };
      if (_value == 0.0) return { _value, 1.0 }; // keeps the sign of -0
      if (_value < 0.0) {                        // sin odd, cos even
        const SinCosPair r = SinCosConst(-_value);
        return { -r.sin, r.cos };
      }

      constexpr double INVPIO2 = 6.36619772367581382433e-01; // 2/π
      constexpr double PIO2_T = -1.49738490485916979800e-33; // π/2 − PIO2DD, the next 53 bits

      const double k = RoundHalfAway(_value * INVPIO2);
      /* k·π/2 as a double-double, subtracted from the argument as one: what is left is the reduced argument to
       * ~106 bits, split into a head `x` and a tail `y` that the polynomials below spend where it matters. Dekker's
       * split would overflow past 2^996, so beyond that - where no accuracy is left to keep anyway - the products
       * are plain doubles. */
      const DD     kp = k < 0x1p990 ? Mul(PIO2DD, k) : DD { k * PIO2DD.hi, k * PIO2DD.lo };
      const DD     r0 = Sub({ _value, 0.0 }, kp);
      const DD     r = FastTwoSum(r0.hi, r0.lo - k * PIO2_T); // r ∈ [−π/4, π/4]
      const double x = r.hi;
      const double y = r.lo;
      const double x2 = x * x;
      const double hx2 = 0.5 * x2;

      /* sin(x + y) ≈ sin x + y cos x: the head's series plus the tail scaled by (1 − x²/2). */
      const double p =
        x2 * (-1.66666666666666666667e-01 +
              x2 * (8.33333333333333333333e-03 +
                    x2 * (-1.98412698412698412698e-04 +
                          x2 * (2.75573192239858906526e-06 +
                                x2 * (-2.50521083854417187751e-08 +
                                      x2 * (1.60590438368216145994e-10 +
                                            x2 * (-7.64716373181981647590e-13 + x2 * 2.81145725434552076320e-15)))))));
      const double s = x + ((y - hx2 * y) + x * p);

      /* cos(x + y) ≈ cos x − y sin x, with cos x = 1 − (x²/2 − x⁴·Q(x²)) so the leading 1 rounds only once. */
      const double q =
        x2 * x2 *
        (4.16666666666666666667e-02 +
         x2 * (-1.38888888888888888889e-03 +
               x2 * (2.48015873015873015873e-05 +
                     x2 * (-2.75573192239858906526e-07 +
                           x2 * (2.08767569878680989792e-09 +
                                 x2 * (-1.14707455977297247139e-11 + x2 * 4.77947733238738529744e-14))))));
      const double c = 1.0 - (hx2 - (q - x * y));

      const double quadrant = k - 4.0 * Trunc(k * 0.25); // k mod 4, exact for any integer-valued double
      if (quadrant == 0.0) return { s, c };
      if (quadrant == 1.0) return { c, -s };
      if (quadrant == 2.0) return { -s, -c };
      return { -c, s };
    }

    /*!
     * \f$\arctan t\f$ for \f$0 \le t \le 1\f$, as a double-double, for the constant-evaluated path only.
     *
     * Two applications of the half-angle identity \f$\arctan t = 2\arctan\frac{t}{1 + \sqrt{1 + t^2}}\f$ bring
     * \f$t\f$ below \f$\tan(\pi/16) \approx 0.199\f$, where the Taylor series \f$t - t^3/3 + t^5/5 - \dots\f$ shrinks
     * by a factor of ~25 per term and twenty-five terms are past \f$2^{-110}\f$. Everything is double-double, so the
     * result is good to ~100 bits and collapses to the correctly rounded angle.
     */
    constexpr DD AtanUnitDD(DD _t) noexcept {
      for (int i = 0; i < 2; ++i) {
        const DD root = SqrtDD(Add({ 1.0, 0.0 }, Mul(_t, _t)));
        _t = Div(_t, Add({ 1.0, 0.0 }, root));
      }
      const DD z = Mul(_t, _t);
      DD       sum = _t;
      DD       term = _t;
      for (int n = 3; n <= 51; n += 2) {
        term = Mul(term, z);
        const DD contribution = Div(term, static_cast<double>(n));
        sum = (n & 2) ? Sub(sum, contribution) : Add(sum, contribution); // signs alternate: −t³/3 +t⁵/5 −t⁷/7 …
      }
      return { 4.0 * sum.hi, 4.0 * sum.lo };
    }

    /*!
     * \f$\arctan(n / d)\f$ for \f$n, d \ge 0\f$, as a double-double, never forming a quotient above 1: the larger
     * operand goes underneath and the complementary angle is taken from \f$\pi/2\f$. `d == 0` is \f$\pi/2\f$
     * (or 0 when `n` is 0 too).
     */
    constexpr DD AngleDD(DD _num, DD _den) noexcept {
      if (_den.hi == 0.0) return _num.hi == 0.0 ? DD { 0.0, 0.0 } : PIO2DD;
      if (_num.hi <= _den.hi) return AtanUnitDD(Div(_num, _den));
      return Sub(PIO2DD, AtanUnitDD(Div(_den, _num)));
    }

    constexpr double AtanConst(double _value) noexcept {
      if (IsNaN(_value) || _value == 0.0) return _value;
      const double ax = Abs(_value);
      /* Past 2^900 the double-double division would overflow Dekker's split; there arctan is π/2 − 1/x with 1/x far
       * below an ulp of π/2, so the subtraction is only kept for its rounding. */
      if (ax > 0x1p900) return __builtin_copysign(Collapse(Sub(PIO2DD, { IsInf(ax) ? 0.0 : 1.0 / ax, 0.0 })), _value);
      return __builtin_copysign(Collapse(AngleDD({ ax, 0.0 }, { 1.0, 0.0 })), _value);
    }

    /*!
     * IEEE 754's `atan2` special-case table, then the angle of the scaled operands. Scaling by a power of two
     * leaves the quotient unchanged and keeps both operands clear of Dekker's split overflowing above \f$2^{996}\f$
     * and of the quotient's tail underflowing below the subnormal range.
     */
    constexpr double Atan2Const(double _y, double _x) noexcept {
      if (IsNaN(_y)) return _y;
      if (IsNaN(_x)) return _x;
      if (_y == 0.0) {
        if (_x > 0.0 || (_x == 0.0 && !__builtin_signbit(_x))) return _y; // ±0
        return __builtin_copysign(Collapse(PIDD), _y);                    // ±π
      }
      if (_x == 0.0) return __builtin_copysign(Collapse(PIO2DD), _y);
      if (IsInf(_x)) {
        if (IsInf(_y)) {
          const DD pio4 = { 0.5 * PIO2DD.hi, 0.5 * PIO2DD.lo };
          return __builtin_copysign(Collapse(_x > 0.0 ? pio4 : Sub(PIDD, pio4)), _y);
        }
        return __builtin_copysign(_x > 0.0 ? 0.0 : Collapse(PIDD), _y);
      }
      if (IsInf(_y)) return __builtin_copysign(Collapse(PIO2DD), _y);

      double       ay = Abs(_y);
      double       ax = Abs(_x);
      const double largest = Max(ay, ax);
      if (largest > 0x1p500) {
        ay = Scale(ay, -600);
        ax = Scale(ax, -600);
      } else if (largest < 0x1p-500) {
        ay = Scale(ay, 600);
        ax = Scale(ax, 600);
      }
      DD angle = AngleDD({ ay, 0.0 }, { ax, 0.0 });
      if (_x < 0.0) angle = Sub(PIDD, angle);
      return __builtin_copysign(Collapse(angle), _y);
    }

    /*!
     * \f$\arcsin x = \arctan\frac{x}{\sqrt{1 - x^2}}\f$ and \f$\arccos x = \arctan\frac{\sqrt{1 - x^2}}{x}\f$,
     * with \f$1 - x^2\f$ formed exactly as a double-double so nothing is lost near ±1. `AngleDD` handles the
     * division by zero at the ends and the quotient's magnitude.
     */
    constexpr double AsinConst(double _value) noexcept {
      if (IsNaN(_value) || _value == 0.0) return _value;
      const double ax = Abs(_value);
      if (ax > 1.0) return __builtin_nan("");
      if (ax == 1.0) return __builtin_copysign(Collapse(PIO2DD), _value);
      const DD root = SqrtDD(Sub({ 1.0, 0.0 }, TwoProd(ax, ax)));
      return __builtin_copysign(Collapse(AngleDD({ ax, 0.0 }, root)), _value);
    }
    constexpr double AcosConst(double _value) noexcept {
      if (IsNaN(_value)) return _value;
      const double ax = Abs(_value);
      if (ax > 1.0) return __builtin_nan("");
      if (_value == 1.0) return 0.0;
      if (_value == -1.0) return Collapse(PIDD);
      const DD root = SqrtDD(Sub({ 1.0, 0.0 }, TwoProd(ax, ax)));
      DD       angle = AngleDD(root, { ax, 0.0 });
      if (_value < 0.0) angle = Sub(PIDD, angle);
      return Collapse(angle);
    }

    /*! The float versions compute in double and narrow; see the note in `exponential.h`. */
    constexpr float AsinConst(float _value) noexcept {
      return static_cast<float>(AsinConst(static_cast<double>(_value)));
    }
    constexpr float AcosConst(float _value) noexcept {
      return static_cast<float>(AcosConst(static_cast<double>(_value)));
    }
    constexpr float AtanConst(float _value) noexcept {
      return static_cast<float>(AtanConst(static_cast<double>(_value)));
    }
    constexpr float Atan2Const(float _y, float _x) noexcept {
      return static_cast<float>(Atan2Const(static_cast<double>(_y), static_cast<double>(_x)));
    }
  } // namespace detail

  /*!
   * Trigonometric functions usable in a constant expression.
   *
   * Like @ref Sqrt, each branches on `__builtin_is_constant_evaluated()`: at runtime it lowers to the hardware/libm
   * intrinsic (`__builtin_sin` and friends), which is the accurate path; at compile time it falls back to the
   * range-reduce-and-series implementation in @ref detail::SinCosConst. Arguments are in radians.
   *
   * @warning The two paths need not agree in the last bit: the compile-time path is within 1 ulp of the true value
   *          (2 ulps in rare cases past \f$2^{48}\f$), and so is libm, but they can land on different neighbours.
   *          The compile-time path also loses accuracy for arguments beyond roughly \f$2^{50}\f$. Don't
   *          `static_assert` a folded result against a decimal literal, and don't assume a value cached at compile
   *          time matches the same call made at run time.
   */
  constexpr double Sin(double _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::SinCosConst(_value).sin;
    return __builtin_sin(_value);
  }
  constexpr float Sin(float _value) noexcept {
    if (__builtin_is_constant_evaluated()) return static_cast<float>(detail::SinCosConst(_value).sin);
    return __builtin_sinf(_value);
  }
  constexpr double Cos(double _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::SinCosConst(_value).cos;
    return __builtin_cos(_value);
  }
  constexpr float Cos(float _value) noexcept {
    if (__builtin_is_constant_evaluated()) return static_cast<float>(detail::SinCosConst(_value).cos);
    return __builtin_cosf(_value);
  }
  constexpr double Tan(double _value) noexcept {
    if (__builtin_is_constant_evaluated()) {
      const detail::SinCosPair sc = detail::SinCosConst(_value);
      return sc.sin / sc.cos;
    }
    return __builtin_tan(_value);
  }
  constexpr float Tan(float _value) noexcept {
    if (__builtin_is_constant_evaluated()) {
      const detail::SinCosPair sc = detail::SinCosConst(_value);
      return static_cast<float>(sc.sin / sc.cos);
    }
    return __builtin_tanf(_value);
  }

  /*!
   * Inverse trigonometry, in radians, usable in a constant expression.
   *
   * At runtime each lowers to its libm builtin. At compile time all four reduce to one arctangent of a quotient no
   * larger than 1, evaluated in double-double by @ref detail::AtanUnitDD, so they fold to the correctly rounded
   * angle; unlike @ref Sin and @ref Cos they agree with libm wherever libm is itself correctly rounded.
   *
   * `Atan2` takes its arguments in the conventional order - `Atan2(y, x)` - and is quadrant-correct across all four,
   * which is what makes it the right tool for recovering an angle from a pair of matrix entries whose signs carry the
   * quadrant. It follows the IEEE 754 table for zeros and infinities on both paths. Mixed and integral arguments
   * promote to double.
   *
   * @param _value for `Asin` and `Acos`, must be in [-1, 1]; a value pushed outside by rounding gives a NaN rather
   *               than a clamped angle, so clamp before calling.
   */
  constexpr double Asin(double _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::AsinConst(_value);
    return __builtin_asin(_value);
  }
  constexpr float Asin(float _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::AsinConst(_value);
    return __builtin_asinf(_value);
  }
  constexpr double Acos(double _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::AcosConst(_value);
    return __builtin_acos(_value);
  }
  constexpr float Acos(float _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::AcosConst(_value);
    return __builtin_acosf(_value);
  }
  constexpr double Atan(double _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::AtanConst(_value);
    return __builtin_atan(_value);
  }
  constexpr float Atan(float _value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::AtanConst(_value);
    return __builtin_atanf(_value);
  }
  constexpr double Atan2(double _y, double _x) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::Atan2Const(_y, _x);
    return __builtin_atan2(_y, _x);
  }
  constexpr float Atan2(float _y, float _x) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::Atan2Const(_y, _x);
    return __builtin_atan2f(_y, _x);
  }
  template <StdScalar A, StdScalar B>
  constexpr double Atan2(A _y, B _x) noexcept {
    return Atan2(static_cast<double>(_y), static_cast<double>(_x));
  }
} // namespace ROSE::math
