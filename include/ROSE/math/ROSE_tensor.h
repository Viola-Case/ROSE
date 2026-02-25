/**

  @file      ROSE_tensor.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/ROSE_preamble.h>

namespace ROSE::math {

  template<typename Derived, Scalar T, size_t ...Dims>
  struct TensorBase {
    constexpr T &operator[](size_t i) {
      return static_cast<Derived *>(this)->data[i];
    }
  };

  template <Scalar T, size_t ...Dims>
  struct Tensor {
  protected:
    static constexpr size_t Rank = sizeof...(Dims);
    static constexpr size_t dims[Rank] = { Dims... };
    static constexpr size_t size = (Dims * ...);
    T data[size];

    template<size_t... Indices>
    static consteval size_t offset_ct() {
      static_assert(sizeof...(Indices) == rank);

      constexpr size_t idx[] = { Indices... };

      size_t off = 0;
      for (size_t i = 0; i < rank; ++i)
        off = off * dims[i] + idx[i];

      return off;
    }

  public:
    template<typename... Indices>
      requires (sizeof...(Indices) == sizeof...(Dims))
    constexpr T &operator()(Indices... idx) {
      
    }
    template<size_t ...idx>
      requires(sizeof...(idx) == sizeof...(Dims))
    constexpr T &at() {

      
    }

    template<size_t... Indices>
    constexpr T &get() noexcept {
      return data[offset_ct<Indices...>()];
    }
  };

}