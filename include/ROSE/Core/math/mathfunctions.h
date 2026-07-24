/**

  @file      mathfunctions.h
  @brief
  @details   ~
  @author    Viola Case
  @date      11.03.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/typetraits.h>
#include <ROSE/Core/math/constants.h>

namespace ROSE::math {
  template <StdScalar T>
  constexpr T Clamp(T value, T min, T max) noexcept {
    return (value > max ? max : (value < min ? min : value));
  }

  /*!
   * @brief Implementation details of the math functions. Not part of the public
   *        API.
   *
   * Anything in here exists to serve a function in @ref ROSE::math and may be
   * changed or removed without notice. Nothing outside this header should name
   * it directly; call the wrapper instead, which handles the dispatch and the
   * edge cases the helpers assume have already been ruled out.
   */
  namespace detail {
    /*!
     * Square root by hand, for the constant-evaluated path only.
     *
     * Seeds \f$1/\sqrt{v}\f$ by halving the exponent in place (the Quake trick,
     * with the double-precision magic constant), then refines with Newton steps
     * on the *inverse* root, which keeps the loop division-free. The seed is
     * good to ~6 bits and every step doubles that, so four steps saturate a
     * double's 53. One classical step at the end brings it to within an ulp of
     * the true root — close, but not the correctly-rounded result the hardware
     * instruction gives; see the note on @ref Sqrt.
     */
    constexpr double SqrtConst(double v) noexcept {
      if (!(v > 0.0)) return (v < 0.0 ? __builtin_nan("") : v); // ±0, NaN, negatives
      if (v > MAXFINITE64) return v;                            // +∞
      // Subnormals seed badly, so scale them into the normal range first.
      if (v < MINNORMAL64) return SqrtConst(v * SUBNORMALSCALE64) * SUBNORMALUNSCALE64;

      double y = __builtin_bit_cast(double, SQRTMAGIC64 - (__builtin_bit_cast(uint64_t, v) >> 1));
      const double h = v * 0.5;
      y *= 1.5 - h * y * y;
      y *= 1.5 - h * y * y;
      y *= 1.5 - h * y * y;
      y *= 1.5 - h * y * y;
      const double r = v * y;
      return 0.5 * (r + v / r);
    }
    constexpr float SqrtConst(float v) noexcept {
      if (!(v > 0.0f)) return (v < 0.0f ? __builtin_nanf("") : v);
      if (v > MAXFINITE32) return v;
      if (v < MINNORMAL32) return SqrtConst(v * SUBNORMALSCALE32) * SUBNORMALUNSCALE32;

      float y = __builtin_bit_cast(float, SQRTMAGIC32 - (__builtin_bit_cast(uint32_t, v) >> 1));
      const float h = v * 0.5f;
      y *= 1.5f - h * y * y;
      y *= 1.5f - h * y * y;
      y *= 1.5f - h * y * y;
      const float r = v * y;
      return 0.5f * (r + v / r);
    }

    /*!
     * Sine and cosine together, for the constant-evaluated path only.
     *
     * `std::sin`/`std::cos` are not constexpr until C++26 and Clang will not
     * fold `__builtin_sin` in a constant expression either, so compile-time
     * evaluation reduces the argument into \f$[-\pi/4, \pi/4]\f$ and sums a
     * Taylor series there. \f$\pi/2\f$ is split into a high part with its low
     * mantissa bits zeroed and a correction term (Cody–Waite), so \f$k\cdot
     * \text{hi}\f$ stays exact and the subtraction keeps the argument's low
     * bits. The engine's @ref ROSE::math::PI is only float-precise (see the
     * note in `constants.h`) and would poison the reduction, so full-precision
     * literals live here instead.
     *
     * Both are returned at once because the quadrant dispatch computes them from
     * the same reduced polynomial; @ref Sin, @ref Cos and @ref Tan all read from
     * this. Accurate to within a couple of ulps for arguments up to roughly
     * \f$2^{20}\f$; beyond that the reduction loses low bits, in the same spirit
     * as @ref SqrtConst — the runtime builtins are the accurate path.
     */
    struct SinCosPair {
      double sin;
      double cos;
    };
    constexpr SinCosPair SinCosConst(double v) noexcept {
      if (v != v) return {v, v};                                          // NaN in, NaN out
      if (v < 0.0) { const SinCosPair r = SinCosConst(-v); return {-r.sin, r.cos}; } // sin odd, cos even
      if (v > MAXFINITE64) return {__builtin_nan(""), __builtin_nan("")}; // ±∞ → NaN

      constexpr double INVPIO2 = 0.636619772367581382433;    // 2/π
      constexpr double PIO2_HI = 1.57079632673412561417;     // π/2, high part (low mantissa bits zero)
      constexpr double PIO2_LO = 6.07710050650619224932e-11; // π/2 − PIO2_HI

      const long long k = static_cast<long long>(v * INVPIO2 + 0.5);
      const double kd = static_cast<double>(k);
      const double r = (v - kd * PIO2_HI) - kd * PIO2_LO; // r ∈ [−π/4, π/4]
      const double r2 = r * r;

      // Taylor on [−π/4, π/4], Horner form. Enough terms for near-double accuracy.
      const double s =
        r * (1.0 + r2 * (-1.66666666666666666e-01 + r2 * (8.33333333333333333e-03 +
        r2 * (-1.98412698412698413e-04 + r2 * (2.75573192239858907e-06 +
        r2 * (-2.50521083854417188e-08 + r2 * 1.60590438368216146e-10))))));
      const double c =
        1.0 + r2 * (-5.00000000000000000e-01 + r2 * (4.16666666666666667e-02 +
        r2 * (-1.38888888888888889e-03 + r2 * (2.48015873015873016e-05 +
        r2 * (-2.75573192239858907e-07 + r2 * 2.08767569878680990e-09)))));

      switch (k & 3) {                 // which quarter-turn the reduction stepped through
        case 0:  return { s,  c};
        case 1:  return { c, -s};
        case 2:  return {-s, -c};
        default: return {-c,  s};
      }
    }
  } // namespace detail

  /*!
   * Square root usable in a constant expression.
   *
   * `std::sqrt` is not constexpr until C++26 and Clang will not fold
   * `__builtin_sqrt` either, so constant evaluation goes through the manual
   * refinement above. At runtime the builtin lowers to a single hardware
   * `sqrt`, which is as good as it gets.
   *
   * @warning The two paths do not always agree in the last bit. The hardware
   *          instruction is correctly rounded; the constant-evaluated
   *          refinement lands 1 ulp either side of it for roughly a quarter of
   *          inputs, in both precisions, though never by more than 1 ulp.
   *          Perfect squares are always exact, so `Sqrt(4.0) == 2.0` holds, but
   *          `Sqrt(2.0)` folds to `1.4142135623730949` where the runtime call
   *          gives `1.4142135623730951`. Don't `static_assert` a folded `Sqrt`
   *          against a decimal literal, and don't assume a value cached at
   *          compile time matches the same computation done at run time.
   */
  constexpr double Sqrt(double value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::SqrtConst(value);
    return __builtin_sqrt(value);
  }
  constexpr float Sqrt(float value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::SqrtConst(value);
    return __builtin_sqrtf(value);
  }
  template <typename T>
  constexpr const T &Min(const T &a, const T &b) noexcept
    requires(std::is_arithmetic_v<T>)
  { return (b < a ? b : a); }
  template <typename T>
  constexpr const T &Max(const T &a, const T &b) noexcept
    requires(std::is_arithmetic_v<T>)
  { return (a < b ? b : a); }

  /*!
   * Trigonometric functions usable in a constant expression.
   *
   * Like @ref Sqrt, each branches on `__builtin_is_constant_evaluated()`: at
   * runtime it lowers to the hardware/libm intrinsic (`__builtin_sin` and
   * friends), which is the accurate path; at compile time it falls back to the
   * manual range-reduce-and-Taylor implementation in @ref detail::SinCosConst.
   * Arguments are in radians.
   *
   * @warning As with @ref Sqrt, the two paths need not agree in the last bit,
   *          and the compile-time path loses accuracy for very large arguments.
   *          Don't `static_assert` a folded result against a decimal literal.
   */
  constexpr double Sin(double value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::SinCosConst(value).sin;
    return __builtin_sin(value);
  }
  constexpr float Sin(float value) noexcept {
    if (__builtin_is_constant_evaluated()) return static_cast<float>(detail::SinCosConst(value).sin);
    return __builtin_sinf(value);
  }
  constexpr double Cos(double value) noexcept {
    if (__builtin_is_constant_evaluated()) return detail::SinCosConst(value).cos;
    return __builtin_cos(value);
  }
  constexpr float Cos(float value) noexcept {
    if (__builtin_is_constant_evaluated()) return static_cast<float>(detail::SinCosConst(value).cos);
    return __builtin_cosf(value);
  }
  constexpr double Tan(double value) noexcept {
    if (__builtin_is_constant_evaluated()) {
      const detail::SinCosPair sc = detail::SinCosConst(value);
      return sc.sin / sc.cos;
    }
    return __builtin_tan(value);
  }
  constexpr float Tan(float value) noexcept {
    if (__builtin_is_constant_evaluated()) {
      const detail::SinCosPair sc = detail::SinCosConst(value);
      return static_cast<float>(sc.sin / sc.cos);
    }
    return __builtin_tanf(value);
  }





} // namespace ROSE::math