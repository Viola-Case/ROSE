/**

  @file      ROSE_simd.h
  @brief     Runtime SIMD level detection and dispatched vector operations
  @author    Viola Case
  @date      04.05.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <cstdint>
#include <cstddef>

namespace ROSE {

  enum class SimdLevel : uint8_t {
    None  = 0,
    SSE4  = 1,
    AVX   = 2,
    AVX2  = 3,
  };

  SimdLevel DetectSimd() noexcept;

  /**
   * @brief Element-wise int32 addition: dst[i] = a[i] + b[i]
   *
   * Dispatches to the best path detected at startup:
   *   AVX2  — 8 int32s/iter via _mm256_add_epi32
   *   AVX   — 2x 128-bit lanes (AVX has no 256-bit integer ops)
   *   SSE4  — 4 int32s/iter via _mm_add_epi32
   */
  void VecAddI32(int32_t *dst, const int32_t *a, const int32_t *b, size_t n) noexcept;

}
