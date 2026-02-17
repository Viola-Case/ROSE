/**

    @file      ROSE_fixedarray.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      16.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/preamble/ROSE_stdlib.h>
#include <ROSE/rtl/ROSE_list.h>

namespace ROSE {
  template <typename T, size_t N>
  struct FixedArray {
    T _data[N];
  public:
    constexpr T *data() noexcept { return _data; }
    constexpr const T *data() noexcept { return _data; }
    constexpr size_t size() noexcept { return N; }

    constexpr T &operator[](size_t i) noexcept { return data_[i]; }
    constexpr const T &operator[](size_t i) const noexcept { return data_[i]; }

    

    constexpr const T *begin() const noexcept { return data(); }
    constexpr const T *end()   const noexcept { return data() + N; }
  };


}