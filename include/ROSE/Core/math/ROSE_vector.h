/**

  @file      ROSE_vector.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_preamble.h>
#include <ROSE/Core/math/ROSE_mathenum.h>


namespace ROSE::math {

  

  template<Scalar T, size_t N>
    requires (N > 1)
  struct Vec {
    FixedArray<T,N> data;
    Vec() = default;
    constexpr const T dot(const Vec &rhs) const noexcept {
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
    constexpr Vec operator+(const Vec &rhs) const noexcept {
      Vec result(*this);
      return (result += rhs);
    }
    constexpr Vec operator-(const Vec &rhs) const noexcept {
      Vec result(*this);
      return (result -= rhs);
    }

    constexpr const T &operator[](const size_t idx) {
      ROSE_ASSERT(idx < N);
      ROSE_ASSERT_MSG(data != nullptr, "Vector storage must not be null");
      return data[idx];
    }
    constexpr T &operator[](const size_t idx) {
      ROSE_ASSERT(idx < N);
      ROSE_ASSERT_MSG(data != nullptr, "Vector storage must not be null");
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
    Vec() = default;
    union {
      FixedArray<T, N> data;
      struct {
        T x, y;
      };
    };

    //constexpr Vec() = default;

    constexpr Vec(T _x = T{}, T _y = T{}) : x(_x), y(_y) {}

    constexpr const T dot(const Vec &rhs) const noexcept {
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
    constexpr Vec operator+(const Vec &rhs) const noexcept {
      Vec result(*this);
      return (result += rhs);
    }
    constexpr Vec operator-(const Vec &rhs) const noexcept {
      Vec result(*this);
      return (result -= rhs);
    }

    constexpr T &operator[](const size_t idx) {
      ROSE_ASSERT(idx < N);
      ROSE_ASSERT_MSG(data != nullptr, "Vector storage must not be null");
      return data[idx];
    }
    template<size_t I>
      requires (I < N)
    constexpr T &at() noexcept {
      return data[I];
    }

    //const 
  };

  template<Scalar T>
  struct Vec<T, 3> {
    static constexpr size_t N = 3;
    Vec() = default;
    union {
      FixedArray<T,N> data;
      struct {
        T x, y, z;
      };
    };

    constexpr Vec(T _x = T{0}, T _y = T{0}, T _z = T{0}) : x(_x), y(_y), z(_z) {}

    constexpr const T dot(const Vec &rhs) const noexcept {
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
    constexpr Vec operator+(const Vec &rhs) const noexcept {
      Vec result(*this);
      return (result += rhs);
    }
    constexpr Vec operator-(const Vec &rhs) const noexcept {
      Vec result(*this);
      return (result -= rhs);
    }

    constexpr T &operator[](const size_t idx) {
      ROSE_ASSERT(idx < N);
      ROSE_ASSERT_MSG(data != nullptr, "Vector storage must not be null");
      return data[idx];
    }
    template<size_t I>
      requires (I < N)
    constexpr T &at() noexcept {
      return data[I];
    }

    constexpr Vec<T, 3> cross(const Vec<T, 3> &rhs) const noexcept {
      return {
        y * rhs.z - z * rhs.y,
        z * rhs.x - x * rhs.z,
        x * rhs.y - y * rhs.x
      };
    }
  };

  template<Scalar T> using Vec2 = Vec<T, 2>;
  template<Scalar T> using Vec3 = Vec<T, 3>;

  using Vec2f = Vec<float, 2>;
  using Vec2d = Vec<double, 2>;
  using Vec3f = Vec<float, 3>;
  using Vec3d = Vec<double, 3>;
}