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
    constexpr Comp(T re_) : Re(re_), Im(T{}) {}
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

    //static inline const Comp I{ 0, -1 };



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
      return (result *= rhs);
    }
    constexpr const Comp operator*(const T &rhs) {
      Comp result(*this);
      return (result *= rhs);
    }
    constexpr const Comp operator/(const Comp &rhs) {
      Comp result(*this);
      return (result /= rhs);
    }
    constexpr const Comp operator/(const T &rhs) {
      Comp result(*this);
      return (result /= rhs);
    }


  };

  using Compf = Comp<float>;
  using Compd = Comp<double>;
}