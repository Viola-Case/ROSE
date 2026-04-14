/**

    @file      ROSE_pair.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      14.04.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <ROSE/Core/rtl/ROSE_utility.h>

namespace ROSE {
  template<class T1, class T2>
  struct Pair {
    T1 first;
    T2 second;
    Pair(const Pair &) = default;
    Pair(Pair &&) = default;
    constexpr Pair() = default;
    Pair(T1 _first, T2 _second) : first(_first), second(_second) {}

    void swap(Pair &other) noexcept {
      Swap(first,other.first);
      Swap(second,other.second);
    }
  };
}