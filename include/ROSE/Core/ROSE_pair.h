/**

    @file      ROSE_pair.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      14.04.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <ROSE/Core/ROSE_utility.h>

namespace ROSE {
  template<class T1, class T2>
  struct Pair {
    T1 first;
    T2 second;

    constexpr Pair() = default;
    Pair(const Pair &) = default;
    Pair(Pair &&) = default;
    Pair &operator=(const Pair &) = default;
    Pair &operator=(Pair &&) = default;

    template<typename U1, typename U2>
    Pair(U1 &&_first, U2 &&_second)
      : first(Forward<U1>(_first)), second(Forward<U2>(_second)) {}

    bool operator==(const Pair &other) const {
      return first == other.first && second == other.second;
    }

    auto operator<=>(const Pair &other) const {
      if (auto cmp = first <=> other.first; cmp != 0) return cmp;
      return second <=> other.second;
    }

    void swap(Pair &other) noexcept {
      Swap(first, other.first);
      Swap(second, other.second);
    }
  };

  template<typename T1, typename T2>
  Pair<T1, T2> MakePair(T1 &&first, T2 &&second) {
    return Pair<T1, T2>(Forward<T1>(first), Forward<T2>(second));
  }
}