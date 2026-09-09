/**

  @file      common.h
  @brief
  @details   ~
  @author    Viola Case
  @date      09.09.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/typetraits.h>

/*!
 * The scalar helpers that need no series, no reduction and no special cases: absolute value, min, max, clamp and the
 * floating-point classifiers. Everything here is a single comparison or a single bit operation, folds in a constant
 * expression as written, and gives the same answer at compile time and at run time.
 */
namespace ROSE::math {
  /*!
   * \f$|value|\f$.
   *
   * For floating point this clears the sign bit, so `Abs(-0.0)` is `+0.0` and a NaN keeps its payload; both
   * `__builtin_fabs` and its `f`/`l` variants fold in a constant expression and lower to a single `and` at run time.
   * For integers it is the usual compare-and-negate, which leaves the unsigned types untouched.
   *
   * @warning `Abs` of the most negative value of a signed integer type is undefined, exactly as `std::abs` is.
   */
  template <StdScalar T>
  constexpr T Abs(T _value) noexcept {
    if constexpr (std::same_as<T, float>) return __builtin_fabsf(_value);
    else if constexpr (std::same_as<T, double>) return __builtin_fabs(_value);
    else if constexpr (std::same_as<T, long double>) return __builtin_fabsl(_value);
    else if constexpr (std::is_signed_v<T>) return static_cast<T>(_value < T { 0 } ? -_value : _value);
    else return _value;
  }

  /*!
   * The smaller / larger of two values, by value.
   *
   * Scalars go by value on purpose: a `const T &` return would bind to a temporary and dangle in
   * `const int &r = Min(a, 5);`, and a reference to a register-sized value is a pessimisation, not an optimisation. If
   * either argument is NaN the *first* is returned, matching `std::min`/`std::max`, so a NaN in the second slot is
   * swallowed - clamp with @ref Clamp if that matters.
   *
   * @note `ROSE::Min`/`ROSE::Max` in `utility.h` are unconstrained templates with the same shape; an unqualified call
   *       with both namespaces in scope is ambiguous. Qualify.
   */
  template <StdScalar T>
  constexpr T Min(T _a, T _b) noexcept {
    return (_b < _a ? _b : _a);
  }
  template <StdScalar T>
  constexpr T Max(T _a, T _b) noexcept {
    return (_a < _b ? _b : _a);
  }

  /*!
   * `value` pulled into `[min, max]`. A NaN `value` fails both comparisons and comes back unchanged, so NaN propagates
   * rather than silently becoming a bound.
   *
   * @param _min lower bound; must not exceed `_max`, which is not checked.
   */
  template <StdScalar T>
  constexpr T Clamp(T _value, T _min, T _max) noexcept {
    return (_value > _max ? _max : (_value < _min ? _min : _value));
  }

  /*!
   * Floating-point classification. All three go through the compiler builtins, which fold in a constant expression
   * and compile to a compare-and-mask at run time. They are the right way to ask; `value != value` also spots a NaN
   * but is the kind of thing `-ffast-math` is entitled to fold to `false`.
   */
  template <std::floating_point T>
  constexpr bool IsNaN(T _value) noexcept {
    return __builtin_isnan(_value);
  }
  template <std::floating_point T>
  constexpr bool IsInf(T _value) noexcept {
    return __builtin_isinf(_value);
  }
  template <std::floating_point T>
  constexpr bool IsFinite(T _value) noexcept {
    return __builtin_isfinite(_value);
  }
} // namespace ROSE::math
