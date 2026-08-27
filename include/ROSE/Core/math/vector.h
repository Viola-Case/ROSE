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
#include <ROSE/Core/math/mathenum.h>
#include <ROSE/Core/math/mathfunctions.h>

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

    /* The cross product's symbol is mostly zeros: of the N*N (j, k) pairs feeding one output component,
     * only two survive in 3D and six in 7D. Contracting over all of them costs real instructions -- a
     * multiply by a zero coefficient is not removable under IEEE semantics, and in 7D the 343-iteration
     * nest is past what the unroller will touch, so the symbol stops folding altogether. Sieving the
     * non-zero terms once, at compile time, keeps the definition as the source of truth and leaves the
     * hot loop with nothing but the terms that actually contribute. */
    struct CrossTerm {
      size_t j, k;
      int8_t sign;
    };

    template <size_t N>
    constexpr Sign CrossSymbol(size_t i, size_t j, size_t k) noexcept {
      if constexpr (N == 3) {
        return LeviCivita(i, j, k);
      } else {
        return FanoSign(i, j, k);
      }
    }

    template <size_t N>
    struct CrossTable {
      static constexpr size_t PerRow = (N == 3 ? 2 : 6);
      CrossTerm terms[N][PerRow] {};
    };

    /* Not noexcept: the throw is unreachable at run time, since consteval turns a wrong support count
     * into a compile error -- it is the sieve asserting that the symbol it was handed has the support the
     * table was sized for. */
    template <size_t N>
    consteval CrossTable<N> MakeCrossTable() {
      CrossTable<N> table {};
      for (size_t i = 0; i < N; ++i) {
        size_t n = 0;
        for (size_t j = 0; j < N; ++j) {
          for (size_t k = 0; k < N; ++k) {
            const int8_t s = static_cast<int8_t>(CrossSymbol<N>(i, j, k));
            if (s != 0) table.terms[i][n++] = { j, k, s };
          }
        }
        if (n != CrossTable<N>::PerRow) throw "cross product symbol has the wrong support";
      }
      return table;
    }

    template <size_t N>
    inline constexpr CrossTable<N> crossTable = MakeCrossTable<N>();

  } // namespace detail

  template <Scalar T, size_t N>
    requires(N > 1)
  struct Vec : detail::VecStorage<T, N> {
    using Storage = detail::VecStorage<T, N>;
    using Storage::data;
    using Storage::Storage;

    static constexpr size_t size = N;

    constexpr Vec() noexcept = default;


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

    constexpr T Dot(const Vec &rhs) const noexcept {
      T sum { 0 };
      for (size_t i = 0; i < N; ++i)
        sum += data[i] * rhs.data[i];
      return sum;
    }

    constexpr Vec Cross(const Vec &rhs) const noexcept
      requires(N == 3 || N == 7)
    {
      /* (a x b)_i = phi_ijk a_j b_k, with the same contraction in both dimensions and only the symbol
       * differing. In 3D phi is the Levi-Civita symbol, which zeroes every term but the two per row that
       * survive. In 7D -- the only other dimension admitting a bilinear cross product -- it is the
       * octonion structure constant, supported on the seven lines of the Fano plane. Both are sieved down
       * to their surviving terms by detail::crossTable, so the sum below runs over those alone. */
      Vec out;
      for (size_t i = 0; i < N; ++i) {
        T acc = T {};
        for (const auto &term : detail::crossTable<N>.terms[i]) {
          if (term.sign > 0) {
            acc += data[term.j] * rhs.data[term.k];
          } else {
            acc -= data[term.j] * rhs.data[term.k];
          }
        }
        out.data[i] = acc;
      }
      return out;
    }

    constexpr T Norm() {
      T sum{0};
      for (size_t idx = 0; idx < N; ++idx) {
        sum += data[idx]*data[idx];
      }
      return Sqrt(sum);
    }

    constexpr Vec Unit() {
      const T n = Norm();
      Vec out;
      for (size_t idx = 0; idx < N; ++idx) {
        out.data[idx] = data[idx] / n;
      }
      return out;
    }

    static constexpr T DotProduct(const Vec &lhs, const Vec &rhs) { return lhs.Dot(rhs); }

    static constexpr Vec CrossProduct(const Vec &lhs, const Vec &rhs) { return lhs.Cross(rhs); }
  };

  template<Scalar T, size_t N>
  constexpr T operator *(const T lhs, const Vec<T, N> &rhs) noexcept {
    return rhs * lhs;
  }

  template <Scalar T>
  using Vec2 = Vec<T, 2>;
  template <Scalar T>
  using Vec3 = Vec<T, 3>;
  template <Scalar T>
  using Vec4 = Vec<T, 4>;
  template <Scalar T>
  using Vec7 = Vec<T, 7>;

  using Vec2f = Vec<float, 2>;
  using Vec2d = Vec<double, 2>;
  using Vec3f = Vec<float, 3>;
  using Vec3d = Vec<double, 3>;
  using Vec4f = Vec<float, 4>;
  using Vec4d = Vec<double, 4>;
  using Vec7f = Vec<float, 7>;
  using Vec7d = Vec<double, 7>;
} // namespace ROSE::math


#ifndef ROSE_MATH_NO_FORMAT

/* Spec grammar: `{:[flags][|scalar-spec]}`, with the flags being
 *   `t`  tuple form, `(1, 2, 3)` -- the default
 *   `c`  cartesian form, `(1x +2y -3z)`; only defined for N <= 4
 *   `q`  label the components `i`, `j`, `k` rather than `x`, `y`, `z`; cartesian only
 *   `n`  naked: leave off the enclosing parentheses
 *   `m`  one component per line
 *   `z`  keep zero components, which cartesian form drops by default
 * Anything past the `|` is the spec every component is formatted with, so `{:c|.3f}`
 * gives `(1.000x +2.000y)`. */
template <ROSE::Scalar T, size_t N>
struct std::formatter<ROSE::math::Vec<T, N>> {

  std::string_view nested_spec;

  enum class Form { Tuple, Cartesian } form = Form::Tuple;

  bool naked = false;
  bool multiline = false;
  bool quatstyle = false;
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
    for (; it != end && *it != '}' && *it != '|'; ++it) {
      switch (*it) {
      case 't':
        if (form != Form::Tuple) throw std::format_error { "conflicting forms" };
        break;
      case 'c':
        if constexpr (N > 4) {
          throw std::format_error { "cartesian form is only defined for vectors of at most four components" };
        }
        form = Form::Cartesian;
        break;
      case 'q':
        quatstyle = true;
        break;
      case 'n':
        naked = true;
        break;
      case 'm':
        multiline = true;
        break;
      case 'z':
        incZero = true;
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

  static constexpr bool is_zero(const T &v) {
    if constexpr (requires {
                    { v == T {} } -> std::convertible_to<bool>;
                  }) {
      return v == T {};
    } else {
      return false;
    }
  }

  /* Cartesian form spells the sign itself, so the magnitude is what gets formatted. */
  static constexpr char split_sign(T &v) {
    if constexpr (std::is_signed_v<T>) {
      if (v < T {}) {
        v = static_cast<T>(-v);
        return '-';
      }
    }
    return '+';
  }

  template <class Out>
  Out emit_scalar(Out out, T v) const {
    if (nested_spec.empty()) return std::format_to(out, "{}", v);
    auto args = std::make_format_args(v);
    return std::vformat_to(out, make_fmt(), args);
  }

  template <class Out>
  Out format_tuple(const ROSE::math::Vec<T, N> &val, Out out) const {
    for (size_t i = 0; i < N; ++i) {
      if (i) {
        *out++ = ',';
        *out++ = (multiline ? '\n' : ' ');
      }
      out = emit_scalar(out, val[i]);
    }

    return out;
  }

  template <class Out>
  Out format_cartesian(const ROSE::math::Vec<T, N> &val, Out out) const
    requires(N <= 4)
  {
    constexpr char coordbuf[5] = "wxyz";
    constexpr char quatstylebuf[5] = "\0ijk";
    const char *labels = quatstyle ? quatstylebuf : coordbuf;
    if constexpr (N < 4) labels++;

    bool first = true;
    for (size_t i = 0; i < N; ++i) {
      T value = val[i];
      if (!incZero && is_zero(value)) continue;
      if (!first) {
        *out++ = (multiline ? '\n' : ' ');
        *out++ = split_sign(value);
      }
      out = emit_scalar(out, value);
      if (const char c = labels[i]; c) *out++ = c;
      first = false;
    }

    /* Every component dropped as zero -- the vector is the origin, and something has to stand for it. */
    if (first) out = emit_scalar(out, T {});

    return out;
  }

  template <typename FormatContext>
  auto format(const ROSE::math::Vec<T, N> &val, FormatContext &ctx) const {
    auto out = ctx.out();

    if (!naked) *out++ = '(';

    if constexpr (N <= 4) {
      out = (form == Form::Cartesian) ? format_cartesian(val, out) : format_tuple(val, out);
    } else {
      out = format_tuple(val, out);
    }

    if (!naked) [[likely]]
      *out++ = ')';

    return out;
  }
};



#endif
