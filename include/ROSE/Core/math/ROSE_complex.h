/**

  @file      ROSE_complex.h
  @brief
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_typetraits.h>

namespace ROSE::math {
  /*!
   * @brief Complex number
   * @tparam T - Underlying arithmetic type
   * @
   */
  template <StdScalar T>
  struct Comp {
    union {
      struct {

        T Re, Im;
      };
      T data[2];
    };

    /*!
     * @brief Zero in the complex basis
     */
    constexpr Comp() = default;

    /*!
     * @brief Real value in the complex basis
     * @param re_ Real number
     */
    constexpr Comp(T re_) : Re(re_), Im(T {}) {}
    /*!
     * @brief Complex number construction
     * @param _re Real component
     * @param _im Imaginary component
     */
    constexpr Comp(T _re, T _im) : Re(_re), Im(_im) {}
    /*!
     * @brief Change of basis from \f$\mathbb{R}^2\f$ to \f$\mathbb{C}\f$
     * @param vec Vector in \f$\mathbb{R}^2\f$ basis
     */
    constexpr explicit Comp(Vec2<T> vec) : Re(vec.x), Im(vec.y) {}

    // static inline const Comp I{ 0, -1 };



    constexpr Comp &operator+=(const Comp &rhs) noexcept {
      Re += rhs.Re;
      Im += rhs.Im;
      return *this;
    }

    constexpr Comp &operator+=(const T &rhs) noexcept {
      Re += rhs;
      return *this;
    }

    constexpr Comp &operator-=(const Comp &rhs) noexcept {
      Re -= rhs.Re;
      Im -= rhs.Im;
      return *this;
    }

    constexpr Comp &operator-=(const T &rhs) noexcept {
      Re -= rhs;
      return *this;
    }

    constexpr Comp &operator*=(const Comp &rhs) noexcept {
      const T r = Re * rhs.Re - Im * rhs.Im;
      const T i = Re * rhs.Im + Im * rhs.Re;
      Re = r;
      Im = i;
      return *this;
    }

    constexpr Comp &operator*=(const T &rhs) noexcept {
      Re *= rhs;
      Im *= rhs;
      return *this;
    }

    constexpr Comp &operator/=(const Comp &rhs) noexcept {
      const T denom = rhs.Re * rhs.Re + rhs.Im * rhs.Im;
      // You can later assert/handle denom == 0 if desired
      const T r = (Re * rhs.Re + Im * rhs.Im) / denom;
      const T i = (Im * rhs.Re - Re * rhs.Im) / denom;
      Re = r;
      Im = i;
      return *this;
    }

    constexpr const Comp operator+(const Comp &rhs) const noexcept {
      Comp result(*this);
      return (result += rhs);
    }
    constexpr const Comp operator+(const T &rhs) const noexcept {
      Comp result(*this);
      return (result += rhs);
    }
    constexpr const Comp operator-(const Comp &rhs) const noexcept {
      Comp result(*this);
      return (result -= rhs);
    }
    constexpr const Comp operator-(const T &rhs) const noexcept {
      Comp result(*this);
      return (result -= rhs);
    }
    constexpr const Comp operator*(const Comp &rhs) const noexcept {
      Comp result(*this);
      return (result *= rhs);
    }
    constexpr const Comp operator*(const T &rhs) const noexcept {
      Comp result(*this);
      return (result *= rhs);
    }
    constexpr const Comp operator/(const Comp &rhs) const noexcept {
      Comp result(*this);
      return (result /= rhs);
    }
    constexpr const Comp operator/(const T &rhs) const noexcept {
      Comp result(*this);
      return (result /= rhs);
    }

    template <StdScalar U>
      requires(!std::is_same_v<T, U>)
    constexpr operator Comp<U>() const noexcept {
      return Comp<U> { static_cast<U>(Re), static_cast<U>(Im) };
    }

    explicit operator T() const {}
  };

  template <StdScalar T, StdScalar U>
  constexpr Comp<T> operator+(const T &lhs, const Comp<U> &rhs) noexcept {
    return Comp<T> { lhs, T {} } + static_cast<Comp<T>>(rhs);
  }

  template <StdScalar T>
  constexpr Comp<T> operator-(const T &lhs, const Comp<T> &rhs) noexcept {
    return Comp<T> { lhs, T {} } - rhs;
  }

  using Compf = Comp<float>;
  using Compd = Comp<double>;

  constexpr Comp<long double> operator""_i(long double im) { return { 0.0L, im }; }
  constexpr Comp<unsigned long long> operator""_i(unsigned long long im) { return { 0ULL, im }; }

} // namespace ROSE::math

#ifndef ROSE_MATH_NO_FORMAT

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <string_view>

#ifndef ROSE_HYPOT
#define ROSE_HYPOT(x, y) std::hypot((x), (y))
#endif
#ifndef ROSE_ATAN2
#define ROSE_ATAN2(y, x) std::atan2((y), (x))
#endif

template <ROSE::StdScalar T>
struct std::formatter<ROSE::math::Comp<T>> {

  bool wrap_parens  = false;  // 'p'
  bool show_zero_re = false;  // 'z' — emit 0 for real even when absent
  bool show_zero_im = false;  // 'Z' — emit 0 for imag even when absent
  bool verbose      = false;  // 'v' — Comp{Re=.., Im=..}
  enum class Form { Rect, Euler, Cis } form = Form::Rect;

  // The nested spec (everything after '|') lives in the format-string literal,
  // so a string_view into it is effectively a compile-time format string —
  // constexpr-storable and cheap to splice into the runtime spec below.
  std::string_view nested_spec;

  // Build "{:<spec>}" for one scalar emission. Runtime construction is required
  // because std::format_string's ctor is consteval; we hand it to vformat_to.
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
    auto it = ctx.begin(), end = ctx.end();

    while (it != end && *it != '}' && *it != '|') {
      switch (*it) {
      case 'p':
        wrap_parens = true; break;
      case 'z':
        show_zero_re = true; break;
      case 'Z':
        show_zero_im = true; break;
      case 'v':
        verbose = true; break;
      case 'e':
        if (form != Form::Rect) throw std::format_error { "conflicting forms" };
        form = Form::Euler;
        break;
      case 'c':
        if (form != Form::Rect) throw std::format_error { "conflicting forms" };
        form = Form::Cis;
        break;
      default:
        throw std::format_error { "unknown complex format flag" };
      }
      ++it;
    }

    if (it != end && *it == '|') {
      ++it;
      auto spec_start = it;
      while (it != end && *it != '}') ++it;
      nested_spec = std::string_view { spec_start, it };
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
  Out format_rect(const ROSE::math::Comp<T> &val, Out out) const {
    const bool emit_re = val.Re != T {} || show_zero_re;
    const bool emit_im = val.Im != T {} || show_zero_im;
    if (!emit_re && !emit_im) return emit_scalar(out, T {});

    if (emit_re) {
      out = emit_scalar(out, val.Re);
      if (emit_im) *out++ = (val.Im < T {} ? '-' : '+');
    }
    if (emit_im) {
      const T im = (emit_re && val.Im < T {}) ? static_cast<T>(-val.Im) : val.Im;
      out = emit_scalar(out, im);
      *out++ = 'i';
    }
    return out;
  }

  template <class Out>
  Out format_euler(const ROSE::math::Comp<T> &val, Out out) const {
    const T mag = ROSE_HYPOT(val.Re, val.Im);
    const T arg = ROSE_ATAN2(val.Im, val.Re);
    out          = emit_scalar(out, mag);
    constexpr std::string_view sep = "e^(";
    out          = std::copy(sep.begin(), sep.end(), out);
    out          = emit_scalar(out, arg);
    *out++ = 'i';
    *out++ = ')';
    return out;
  }

  template <class Out>
  Out format_cis(const ROSE::math::Comp<T> &val, Out out) const {
    const T mag = ROSE_HYPOT(val.Re, val.Im);
    const T arg = ROSE_ATAN2(val.Im, val.Re);
    out          = emit_scalar(out, mag);
    constexpr std::string_view sep = "cis(";
    out          = std::copy(sep.begin(), sep.end(), out);
    out          = emit_scalar(out, arg);
    *out++ = ')';
    return out;
  }

  template <class Out>
  Out format_verbose(const ROSE::math::Comp<T> &val, Out out) const {
    constexpr std::string_view pre = "Comp{Re=";
    constexpr std::string_view mid = ", Im=";
    out = std::copy(pre.begin(), pre.end(), out);
    out = emit_scalar(out, val.Re);
    out = std::copy(mid.begin(), mid.end(), out);
    out = emit_scalar(out, val.Im);
    *out++ = '}';
    return out;
  }

  auto format(const ROSE::math::Comp<T> &val, auto &ctx) const {
    auto out = ctx.out();
    if (wrap_parens) *out++ = '(';

    if (verbose) {
      out = format_verbose(val, out);
    } else
      switch (form) {
      case Form::Rect:  out = format_rect(val, out); break;
      case Form::Euler: out = format_euler(val, out); break;
      case Form::Cis:   out = format_cis(val, out); break;
      }

    if (wrap_parens) *out++ = ')';
    return out;
  }
};

#endif
