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

} // namespace ROSE::math