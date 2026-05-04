#include <ROSE/Core/ROSE_simd.h>
#include <ROSE/Core/ROSE_platform.h>

#if ROSE_COMPILER_MSVC
#  include <intrin.h>
#  include <immintrin.h>
#else
#  include <cpuid.h>
#  include <immintrin.h>
#endif

namespace ROSE {

// ---------------------------------------------------------------------------
// CPUID helpers
// ---------------------------------------------------------------------------

namespace {
  struct CpuidResult { int eax, ebx, ecx, edx; };

  CpuidResult Cpuid(int leaf) noexcept {
    CpuidResult r{};
#if ROSE_COMPILER_MSVC
    __cpuid(reinterpret_cast<int *>(&r), leaf);
#else
    __get_cpuid(static_cast<unsigned>(leaf),
                reinterpret_cast<unsigned *>(&r.eax),
                reinterpret_cast<unsigned *>(&r.ebx),
                reinterpret_cast<unsigned *>(&r.ecx),
                reinterpret_cast<unsigned *>(&r.edx));
#endif
    return r;
  }

  CpuidResult CpuidEx(int leaf, int subleaf) noexcept {
    CpuidResult r{};
#if ROSE_COMPILER_MSVC
    __cpuidex(reinterpret_cast<int *>(&r), leaf, subleaf);
#else
    __get_cpuid_count(static_cast<unsigned>(leaf), static_cast<unsigned>(subleaf),
                      reinterpret_cast<unsigned *>(&r.eax),
                      reinterpret_cast<unsigned *>(&r.ebx),
                      reinterpret_cast<unsigned *>(&r.ecx),
                      reinterpret_cast<unsigned *>(&r.edx));
#endif
    return r;
  }
} // namespace

// ---------------------------------------------------------------------------
// Detection
// ---------------------------------------------------------------------------

SimdLevel DetectSimd() noexcept {
  auto leaf1  = Cpuid(1);
  auto leaf7  = CpuidEx(7, 0);

  bool osxsave = (leaf1.ecx >> 27) & 1;
  bool ymmReady = osxsave && ((_xgetbv(0) & 6u) == 6u);

  bool sse41 = (leaf1.ecx >> 19) & 1;
  bool avx   = ymmReady && ((leaf1.ecx >> 28) & 1);
  bool avx2  = avx && ((leaf7.ebx >> 5) & 1);

  if (avx2)  return SimdLevel::AVX2;
  if (avx)   return SimdLevel::AVX;
  if (sse41) return SimdLevel::SSE4;
  return SimdLevel::None;
}

// ---------------------------------------------------------------------------
// VecAddI32 implementations
// ---------------------------------------------------------------------------

namespace {

  // AVX2: true 256-bit integer addition, 8 int32s per iteration
  void VecAddI32_AVX2(int32_t *dst, const int32_t *a, const int32_t *b, size_t n) noexcept {
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
      __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(a + i));
      __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(b + i));
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst + i), _mm256_add_epi32(va, vb));
    }
    for (; i < n; ++i)
      dst[i] = a[i] + b[i];
    _mm256_zeroupper();
  }

  // AVX: no 256-bit integer ops — process two 128-bit lanes manually
  void VecAddI32_AVX(int32_t *dst, const int32_t *a, const int32_t *b, size_t n) noexcept {
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
      __m128i va_lo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(a + i));
      __m128i vb_lo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(b + i));
      __m128i va_hi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(a + i + 4));
      __m128i vb_hi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(b + i + 4));
      _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + i),      _mm_add_epi32(va_lo, vb_lo));
      _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + i + 4),  _mm_add_epi32(va_hi, vb_hi));
    }
    for (; i < n; ++i)
      dst[i] = a[i] + b[i];
  }

  // SSE4.1: 4 int32s per iteration; SSE4.1 class hardware also enables _mm_mullo_epi32 etc.
  void VecAddI32_SSE4(int32_t *dst, const int32_t *a, const int32_t *b, size_t n) noexcept {
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
      __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i *>(a + i));
      __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i *>(b + i));
      _mm_storeu_si128(reinterpret_cast<__m128i *>(dst + i), _mm_add_epi32(va, vb));
    }
    for (; i < n; ++i)
      dst[i] = a[i] + b[i];
  }

  // Scalar fallback when no SIMD is available
  void VecAddI32_Scalar(int32_t *dst, const int32_t *a, const int32_t *b, size_t n) noexcept {
    for (size_t i = 0; i < n; ++i)
      dst[i] = a[i] + b[i];
  }

  using VecAddI32Fn = void(*)(int32_t *, const int32_t *, const int32_t *, size_t) noexcept;

  VecAddI32Fn PickVecAddI32() noexcept {
    switch (DetectSimd()) {
      case SimdLevel::AVX2: return VecAddI32_AVX2;
      case SimdLevel::AVX:  return VecAddI32_AVX;
      case SimdLevel::SSE4: return VecAddI32_SSE4;
      default:              return VecAddI32_Scalar;
    }
  }

  // Resolved once at startup
  VecAddI32Fn g_VecAddI32 = PickVecAddI32();

} // namespace

void VecAddI32(int32_t *dst, const int32_t *a, const int32_t *b, size_t n) noexcept {
  g_VecAddI32(dst, a, b, n);
}

} // namespace ROSE
