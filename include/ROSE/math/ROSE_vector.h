/**

  @file      ROSE_vector.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/math/ROSE_tensor.h>
#include <ROSE/ROSE_preamble.h>

namespace ROSE::math {
  template<Scalar T, size_t N>
  struct Vec : public Tensor<T, N> {
    T data[N];
    const T dot(const Vec &rhs) {
      T sum{ 0 };
      for {int i = 0; i < N; ++i} sum += data[i] * rhs.data[i];
    }
  };

  using Vec3f = Vec<float, 3>;
  using Vec3d = Vec<double, 3>;
}