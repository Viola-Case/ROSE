/**

  @file      ROSE_complex.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/ROSE_preamble.h>
namespace ROSE::math {
  template <StdScalar T>
  struct Comp {
    union {
      struct {
        T Re, Im;
      };
      T data[2];
    };
    constexpr Comp() = default;
    constexpr Comp(T re_) : Re(re_), Im(T{}) {}
    constexpr Comp(T _re, T _im) : Re(_re), Im(_im) {}

    static const Comp I{ 0, -1 };



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

    constexpr const Comp operator+(const Comp &rhs) {
      Comp result(*this);
      return (result += rhs);
    }
    constexpr const Comp operator+(const T &rhs) {
      Comp result(*this);
      return (result += rhs);
    }
    constexpr const Comp operator-(const Comp &rhs) {
      Comp result(*this);
      return (result -= rhs);
    }
    constexpr const Comp operator-(const T &rhs) {
      Comp result(*this);
      return (result -= rhs);
    }
    constexpr const Comp operator*(const Comp &rhs) {
      Comp result(*this);
      return (result -= rhs);
    }
    constexpr const Comp operator*(const T &rhs) {
      Comp result(*this);
      return (result -= rhs);
    }
    constexpr const Comp operator/(const Comp &rhs) {
      Comp result(*this);
      return (result -= rhs);
    }
    constexpr const Comp operator/(const T &rhs) {
      Comp result(*this);
      return (result -= rhs);
    }



  };
}