/**

    @file      array.h
    @brief
    @details   ~
    @author    Viola Case
    @date      16.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/stdlib.h>
#include <ROSE/Core/macros.h>

namespace ROSE {
  template <typename T, size_t N>
  struct FixedArray {
    T _data[N];

  public:
    /* Declaring the constructors below suppresses the implicit default constructor, which used to propagate out to
     * every aggregate holding a FixedArray - Mat and Vec<T, N >= 5> were both impossible to construct because of it.
     * Elements are left default-initialized, like std::array; construct from a list or assign before reading. */
    constexpr FixedArray() noexcept = default;

    template <size_t M>
    constexpr FixedArray(const T (&arr)[M]) {
      static_assert(M == N, "Size mismatch");
      for (size_t i = 0; i < N; ++i)
        _data[i] = arr[i];
    }

    /* A runtime check, not a static_assert: list.size() is not a constant expression, so the static_assert this
     * replaces made the constructor ill-formed the moment it was instantiated. Extra elements are ignored. */
    constexpr FixedArray(std::initializer_list<T> list) {
      ROSE_ASSERT_MSG(list.size() == N, "Size mismatch");
      size_t i = 0;
      for (const auto &v : list) {
        if (i >= N) break;
        _data[i++] = v;
      }
    }

    constexpr T *data() noexcept { return _data; }
    constexpr const T *data() const noexcept { return _data; }
    constexpr size_t size() const noexcept { return N; }

    constexpr T &operator[](size_t i) noexcept { return _data[i]; }
    constexpr const T &operator[](size_t i) const noexcept { return _data[i]; }

    constexpr T *begin() noexcept { return data(); }
    constexpr T *end() noexcept { return data() + N; }
    constexpr const T *begin() const noexcept { return data(); }
    constexpr const T *end() const noexcept { return data() + N; }
  };


} // namespace ROSE