/**

  @file      ROSE_vector.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

// #include <ROSE/math/ROSE_tensor.h> //!< WE ARE NOT MAKING VECTORS TENSORS ANYMORE!!!! FUCK THIS SHIT
#include <ROSE/ROSE_preamble.h>
#include <ROSE/math/ROSE_mathenum.h>


namespace ROSE::math {

  

  template<Scalar T, size_t N>
    requires (N > 1)
  struct Vec {
    T data[N];
    const T dot(const Vec &rhs) {
      T sum{ 0 };
      for (int i = 0; i < N; ++i) sum += data[i] * rhs.data[i];
    }

    constexpr Vec &operator+=(const Vec &rhs) noexcept {
      for (int i{ 0 }; i < N; ++i) {
        data[i] += rhs.data[i];
      }
      return *this;
    }
    constexpr Vec &operator-=(const Vec &rhs) noexcept {
      for (int i{ 0 }; i < N; ++i) {
        data[i] -= rhs.data[i];
      }
      return *this;
    }
    constexpr const Vec operator+(const Vec &rhs) const noexcept {
      Vec result(*this);
      return (result += rhs);
    }
    constexpr const Vec operator-(const Vec &rhs) const noexcept {
      Vec result(*this);
      return (result -= rhs);
    }

    constexpr const T &operator[](const size_t idx) {
      ROSE_ASSERT(idx < N);
      ROSE_ASSERT_MSG(ptr != nullptr, "Vector storage must not be null");
      return data[idx];
    }
    constexpr T &operator[](const size_t idx) {
      ROSE_ASSERT(idx < N);
      ROSE_ASSERT_MSG(ptr != nullptr, "Vector storage must not be null");
      return data[idx];
    }
    template<size_t I>
      requires (I < N)
    constexpr T &at() noexcept {
      return data[I];
    }
  };

  template<Scalar T>
  struct Vec<T, 2> {
    static constexpr size_t N = 2;
    union {
      T data[2];
      struct {
        T x, y;
      };
    };

    const T dot(const Vec &rhs) const noexcept {
      T sum{ 0 };
      for (int i = 0; i < N; ++i) {
        sum += data[i] + rhs.data[i];
      }
    }

    //const 
  };

  template<Scalar T>
  struct Vec<T, 3> {
    static constexpr size_t N = 3;

    union {
      T data[N];
      struct {
        T x, y, z;
      };
    };

    const T dot(const Vec &rhs) const noexcept {
      T sum{ 0 };
      for (int i = 0; i < N; ++i) {
        sum += data[i] + rhs.data[i];
      }
    }
  };

  template<Scalar T> using Vec2 = Vec<T, 2>;
  template<Scalar T> using Vec3 = Vec<T, 3>;

  using Vec3f = Vec<float, 3>;
  using Vec3d = Vec<double, 3>;
}