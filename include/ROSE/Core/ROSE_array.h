/**

    @file      ROSE_fixedarray.h
    @brief
    @details   ~
    @author    Viola Case
    @date      16.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_stdlib.h>

namespace ROSE {
  template <typename T, size_t N>
  struct FixedArray {
    T _data[N];

  public:
    template <size_t M>
    constexpr FixedArray(const T (&arr)[M]) {
      static_assert(M == N, "Size mismatch");
      for (size_t i = 0; i < N; ++i)
        _data[i] = arr[i];
    }

    constexpr FixedArray(std::initializer_list<T> list) {
      static_assert(list.size() == N, "Size mismatch");
      size_t i = 0;
      for (const auto &v : list)
        _data[i++] = v;
    }

    constexpr T *data() noexcept { return _data; }
    // constexpr const T *data() noexcept { return _data; }
    constexpr size_t size() noexcept { return N; }

    constexpr T &operator[](size_t i) noexcept { return _data[i]; }
    constexpr const T &operator[](size_t i) const noexcept { return _data[i]; }

    constexpr const T *begin() const noexcept { return data(); }
    constexpr const T *end() const noexcept { return data() + N; }
  };


} // namespace ROSE