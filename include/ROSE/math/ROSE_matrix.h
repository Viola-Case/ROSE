#pragma once

#include <ROSE/math/ROSE_vector.h>

namespace ROSE::math {
  template<typename T, size_t Rows, size_t Cols>
  class Matrix : public Tensor<T, Rows, Cols> {};
}