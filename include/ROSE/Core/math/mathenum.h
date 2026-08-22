/**

    @file      mathenum.h
    @brief
    @details   ~
    @author    Viola Case
    @date      25.02.2026
    @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <cstdlib>
#include <cstdint>
#include <type_traits>

namespace ROSE::math {
  /**
      @struct Sign
      @brief  it's a sign lol
  **/
  struct Sign {
    enum Value : int8_t { Negative = -1, Zero = 0, Positive = 1 } value;
    constexpr Sign(Value v) : value(v) {}
    constexpr Sign(float i) : value((i == 0 ? Value::Zero : (i > 0 ? Value::Positive : Value::Negative))) {}
    constexpr operator int8_t() const noexcept { return static_cast<int8_t>(value); }
  };

  constexpr Sign SignOf(float num) noexcept {
    return (num > 0.f ? Sign::Positive : (num < 0.f ? Sign::Negative : Sign::Zero));
  }

  constexpr bool IsPositive(const Sign s, bool incZero = true) noexcept {
    return (s == Sign::Positive) || (incZero && s == Sign::Zero);
  }

  constexpr bool IsNegative(const Sign s, bool incZero = false) noexcept {
    return (s == Sign::Negative) || (incZero && s == Sign::Zero);
  }

  /*!
   * Behold, the most useless fucking function in any API in the universe.
   * Rendered obsolete by the comparison operator *_WHICH IT CALLS_*
   *
   *
   */
  template <typename T>
  constexpr bool KDelta(T a, T b) {
    return (a == b);
  }

  /**
    @brief  Returns the sign of the permutation of arguments (aka "Levi-Civita symbol")
    @tparam Args - template parameter pack type
    @param  args - template parameter pack, use `size_t`
    @retval      - sign of the permutation
    @note Why did I put this here? Stupid ass physics major
  **/

  template <typename... Args>
  constexpr Sign LeviCivita(Args... args) {
    static_assert((std::is_convertible_v<Args, size_t> && ...),
                  "cse::math::LeviCivita only accepts size_t-convertible arguments");
    constexpr size_t N = sizeof...(Args);
    size_t arr[N] = { args... };

    /* A non-zero symbol needs a permutation of 0..N-1: every index in range, and no index repeated. */
    for (size_t i = 0; i < N; ++i) {
      if (arr[i] >= N) return Sign::Zero;
      for (size_t j = i + 1; j < N; ++j) {
        if (arr[i] == arr[j]) return Sign::Zero;
      }
    }

    int sign = 1;
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = i + 1; j < N; ++j) {
        if (arr[i] > arr[j]) sign = -sign;
      }
    }

    return (sign == 1 ? Sign::Positive : Sign::Negative);
  }

  /**
    @brief  Returns the octonion structure constant phi_ijk (the "Fano plane" symbol)
    @param  i,j,k - indices in [0, 7)
    @retval       - sign of the triple: non-zero only on the 7 Fano lines and their permutations
    @note The 7D counterpart of LeviCivita: same totally antisymmetric role, but supported on 7 index
          triples instead of on every permutation of a full index set. The basis below is the one where
          (0, 1, 2) is a line, so a 7D cross product reduces to the 3D one when components 3..6 vanish.
  **/

  constexpr Sign FanoSign(size_t i, size_t j, size_t k) {
    constexpr size_t lines[7][3] = {
      { 0, 1, 2 }, { 0, 3, 4 }, { 0, 6, 5 }, { 1, 3, 5 }, { 1, 4, 6 }, { 2, 3, 6 }, { 2, 5, 4 },
    };

    for (const auto &line : lines) {
      /* Locate each index within the line; the parity of those positions is the sign, so LeviCivita does
       * the counting. A missing index means this is not the line, and a repeated index cannot place twice
       * because the entries of a line are distinct. */
      size_t pos[3] = { 3, 3, 3 };
      for (size_t p = 0; p < 3; ++p) {
        if (line[p] == i) pos[0] = p;
        if (line[p] == j) pos[1] = p;
        if (line[p] == k) pos[2] = p;
      }
      if (pos[0] == 3 || pos[1] == 3 || pos[2] == 3) continue;

      return LeviCivita(pos[0], pos[1], pos[2]);
    }

    return Sign::Zero;
  }

} // namespace ROSE::math