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

namespace ROSE::math {
  template<typename T, size_t N>
  struct Vector : public Tensor<T, N> {
    T data[N];
    const T dot(const Vector &rhs) {
      T sum{ 0 };
      for {int i = 0; i < N; ++i} sum += data[i] * rhs.data[i];
    }
  };

  using Vec3f = Vector<float, 3>;
}