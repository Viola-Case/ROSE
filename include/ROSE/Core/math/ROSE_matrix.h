#pragma once

#include <ROSE/Core/math/ROSE_vector.h>

namespace ROSE::math {
  template<Scalar T, size_t Rows, size_t Cols>
  struct Mat {
    static constexpr size_t size = Rows * Cols;
    FixedArray<T,size> data;
    constexpr Mat() noexcept = default;
    constexpr Mat(const Mat &) noexcept = default;

    constexpr Vec<T,Rows> col(size_t idx) const noexcept {
      // runtime assert idx is within needed bounds
      Vec<T, Rows> result;
      for (size_t r = 0; r < Rows; ++r)
        result.data[r] = data[r * Cols + idx];
      return result;
    }

    constexpr Vec<T,Cols> row(size_t idx) const noexcept {
      // runtime assert idx is within needed bounds
      Vec<T, Cols> result;
      for (size_t r = 0; r < Cols; ++r)
        result.data[r] = data[idx * Cols + r];
      return result;
    }

    constexpr T &operator()(const size_t row, const size_t col) noexcept {
      return data[row * Cols + col];
    }

    constexpr Mat &operator+=(const Mat &rhs) noexcept {
      for (int i{0}; i < size; ++i) {
        data[i] += rhs.data[i];
      }
      return *this;
    }

    constexpr Mat &operator-=(const Mat &rhs) noexcept {
      for (int i{0}; i < size; ++i) {
        data[i] -= rhs.data[i];
      }
      return *this;
    }

    constexpr Mat operator*(const Mat &rhs) const noexcept {
      Mat result;
      for (size_t row{0}; row < Rows; ++row) {
        for (size_t col{0}; col < Cols; ++col) {

        }
      }
    }
  };
}