/**

  @file      vector.h
  @brief
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <cstdlib>
#include <ROSE/Core/rtl.h>
#include <ROSE/Core/typetraits.h>


namespace ROSE::math {



  /* Tag for skipping zero-initialization of the storage. */
  struct NoInit {};

  namespace detail {
    /* Only the storage layout and its constructors vary with N: the small vectors overlay named coordinates on the
     * array, and Vec3 pads to four elements for SIMD. Everything else lives once, in Vec below.
     *
     * The component constructors initialize `data` rather than the named members, which makes the array the active
     * union member. Constant evaluation refuses to read the inactive member of a union, so this is what decides which
     * spelling folds: the whole-vector operations below all go through `data`, so they work in a constant expression,
     * at the cost of `v.x` no longer being readable in one. Reading the other name still works at runtime, as an
     * extension every compiler implements. */
    template <Scalar T, size_t N>
    struct VecStorage {
      FixedArray<T, NextPow2(N)> data;

      constexpr VecStorage() noexcept : data {} {}
      explicit constexpr VecStorage(NoInit) noexcept {}

      template <typename... Args>
        requires(sizeof...(Args) > 0 && sizeof...(Args) <= N && (std::convertible_to<Args, T> && ...))
      constexpr VecStorage(Args... args) noexcept : data {} {
        size_t i = 0;
        ((data[i++] = static_cast<T>(args)), ...);
      }
    };

    template <Scalar T>
    struct VecStorage<T, 2> {
      union {
        FixedArray<T, 2> data;
        struct {
          T x, y;
        };
      };

      constexpr VecStorage() noexcept : data {} {}
      explicit constexpr VecStorage(NoInit) noexcept {}
      constexpr VecStorage(T _x, T _y = T {}) noexcept : data { _x, _y } {}
    };

    template <Scalar T>
    struct VecStorage<T, 3> {
      union {
        FixedArray<T, 4> data;
        struct {
          T x, y, z, w; // w added for SIMD padding
        };
      };

      constexpr VecStorage() noexcept : data {} {}
      explicit constexpr VecStorage(NoInit) noexcept {}
      constexpr VecStorage(T _x, T _y = T {}, T _z = T {}) noexcept : data { _x, _y, _z, T {} } {}
    };

    template <Scalar T>
    struct VecStorage<T, 4> {
      union {
        FixedArray<T, 4> data;
        struct {
          T x, y, z, w;
        };
      };

      constexpr VecStorage() noexcept : data {} {}
      explicit constexpr VecStorage(NoInit) noexcept {}
      constexpr VecStorage(T _x, T _y = T {}, T _z = T {}, T _w = T {}) noexcept : data { _x, _y, _z, _w } {}
    };
  } // namespace detail

  template <Scalar T, size_t N>
    requires(N > 1)
  struct Vec : detail::VecStorage<T, N> {
    using Storage = detail::VecStorage<T, N>;
    using Storage::data;
    using Storage::Storage;

    static constexpr size_t size = N;

    constexpr Vec() noexcept = default;

    constexpr T dot(const Vec &rhs) const noexcept {
      T sum { 0 };
      for (size_t i = 0; i < N; ++i)
        sum += data[i] * rhs.data[i];
      return sum;
    }

    constexpr Vec &operator+=(const Vec &rhs) noexcept {
      for (size_t i { 0 }; i < N; ++i) {
        data[i] += rhs.data[i];
      }
      return *this;
    }
    constexpr Vec &operator-=(const Vec &rhs) noexcept {
      for (size_t i { 0 }; i < N; ++i) {
        data[i] -= rhs.data[i];
      }
      return *this;
    }
    constexpr Vec &operator*=(const T rhs) noexcept {
      for (size_t i { 0 }; i < N; ++i) {
        data[i] *= rhs;
      }
      return *this;
    }
    constexpr Vec operator+(const Vec &rhs) const noexcept {
      Vec result(*this);
      return (result += rhs);
    }
    constexpr Vec operator-(const Vec &rhs) const noexcept {
      Vec result(*this);
      return (result -= rhs);
    }
    constexpr Vec operator*(const T rhs) const noexcept {
      Vec result(*this);
      return (result *= rhs);
    }

    constexpr const T &operator[](const size_t idx) const {
      ROSE_ASSERT(idx < N);
      return data[idx];
    }
    constexpr T &operator[](const size_t idx) {
      ROSE_ASSERT(idx < N);
      return data[idx];
    }
    template <size_t I>
      requires(I < N)
    constexpr T &at() noexcept {
      return data[I];
    }
    template <size_t I>
      requires(I < N)
    constexpr const T &at() const noexcept {
      return data[I];
    }

    template <Scalar U>
    constexpr operator Vec<U, N>() const {
      Vec<U, N> v;
      for (size_t i = 0; i < N; ++i) {
        v.data[i] = static_cast<U>(data[i]);
      }
      return v;
    }

    constexpr Vec cross(const Vec &rhs) const noexcept
      requires(N == 3)
    {
      return { data[1] * rhs.data[2] - data[2] * rhs.data[1],
               data[2] * rhs.data[0] - data[0] * rhs.data[2],
               data[0] * rhs.data[1] - data[1] * rhs.data[0] };
    }
  };

  template <Scalar T>
  using Vec2 = Vec<T, 2>;
  template <Scalar T>
  using Vec3 = Vec<T, 3>;
  template <Scalar T>
  using Vec4 = Vec<T, 4>;

  using Vec2f = Vec<float, 2>;
  using Vec2d = Vec<double, 2>;
  using Vec3f = Vec<float, 3>;
  using Vec3d = Vec<double, 3>;
  using Vec4f = Vec<float, 4>;
  using Vec4d = Vec<double, 4>;
} // namespace ROSE::math


#ifndef ROSE_MATH_NO_FORMAT

template <ROSE::Scalar T, size_t N>
struct std::formatter<ROSE::math::Vec<T, N>> {

  std::string_view nested_spec;

  enum class Form { Tuple, Cartesian } form = Form::Tuple;

  bool naked = false;
  bool multiline = false;
  bool quatstyle = false;
  bool verbose = false;
  bool incZero = false;


  std::string make_fmt() const {
    if (nested_spec.empty()) return "{}";
    std::string s;
    s.reserve(nested_spec.size() + 3);
    s += "{:";
    s += nested_spec;
    s += '}';
    return s;
  }

  constexpr auto parse(std::format_parse_context &ctx) {
    auto it = ctx.begin();
    const auto end = ctx.end();
    while (it != end && *it != '}' && *it != '|') {
      switch (*it) {
      case 'c':
        if (form != Form::Tuple) throw std::format_error { "conflicting forms" };
        form = Form::Cartesian;
        break;
      case 'q':
        quatstyle = true;
        break;
      default:
        throw std::format_error { "unknown vector format flag" };
      }
    }

    if (it != end && *it == '|') {
      ++it;
      auto spec_start = it;
      while (it != end && *it != '}')
        ++it;
      nested_spec = std::string_view { spec_start, it };
    }

    if (quatstyle && form != Form::Cartesian) {
      throw std::format_error { "conflicting flags! quaternion style only compatible with cartesian form!" };
    }

    return it;
  }

  template <class Out>
  Out emit_scalar(Out out, T v) const {
    if (nested_spec.empty()) return std::format_to(out, "{}", v);
    auto args = std::make_format_args(v);
    return std::vformat_to(out, make_fmt(), args);
  }

  template <class Out>
  auto format_tuple(const ROSE::math::Vec<T, N> &val, Out out) const {
    *out++ = '(';
    for (size_t i = 0; i < N; ++i) {
      emit_scalar(out, val[i]);
      *out++ = ',';
      *out++ = (multiline ? '\n' : ' ');
    }
    *out++ = ')';

    return out;
  }

  template <class Out>
  auto format_cartesian(const ROSE::math::Vec<T, N> &val, Out out) const
    requires(N <= 4)
  {
    constexpr char coordbuf[5] = "wxyz";
    constexpr char quatstylebuf[5] = "\0ijk";
    const char *data = coordbuf;
    if (quatstyle) data = quatstylebuf;
    if constexpr (N < 4) data++;

    if (!naked) [[likely]] {
      *out++ = '(';
    }

    for (int i = 0; i < N && (val.operator[](i) != 0 || incZero); ++i) {
      T value = val.operator[](i);
      if (!value && !incZero) continue;
      char sign = (value >= 0 ? '+' : '-');
      if ((i < N - 1) && i) *out++ = sign;
      emit_scalar(out, val[i]);
      char c { quatstylebuf[i] };
      if (c) *out++ = c;
      *out++ = ' ';
    }

    if (!naked) [[likely]] {
      *--out++ = ')';
    } else {
      *--out = '\0';
    }

    return out;
  }

  template <typename FormatContext>
  auto format(const ROSE::math::Vec<T, N> &val, FormatContext &ctx) const {
    auto out = ctx.out();

    if (!naked) *out++ = '(';

    switch (form) {
    case Form::Tuple:
      out = format_tuple(val, out);
      break;
    case Form::Cartesian:
      out = format_cartesian(val, out);
      break;
    }

    if (!naked) [[likely]]
      *out++ = ')';

    return out;
  }
};



#endif
