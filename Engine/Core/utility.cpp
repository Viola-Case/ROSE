#include <ROSE/ROSE.h>

#include <chrono>
#include <intrin.h>


namespace ROSE {
  void MemCpy(void *_Dst, const void *_Src, size_t size) {
    auto *p = reinterpret_cast<unsigned char *>(_Dst);
    auto *q = reinterpret_cast<const unsigned char *>(_Src);

    while (size--) {
      *p++ = *q++;
    }
  }


  uint64_t FNV1A64(const void *data, size_t len) {
    uint64_t hash = math::FNVOFFSET64;
    for (int i = 0; i < len; ++i) {
      hash ^= static_cast<const char8_t *>(data)[i];
      hash *= math::FNVPRIME64;
    }
    return hash;
  }

  uint64_t FNV1A64(const char *str) {
    uint64_t hash = math::FNVOFFSET64;
    auto len = StrLen(str);
    for (int i = 0; i < len; ++i) {
      hash ^= str[i];
      hash *= math::FNVPRIME64;
    }
    return hash;
  }

  uint64_t FNV1A64(const StringView &str) { return FNV1A64(str.c_str()); }

  uint128_t FNV1A128(const void *data, size_t len) {
    uint128_t hash = math::FNVOFFSET128;
    for (size_t i = 0; i < len; ++i) {
      hash ^= static_cast<const uint8_t *>(data)[i];
      hash *= math::FNVPRIME128;
    }
    return hash;
  }

  uint128_t FNV1A128(const char *str) {
    uint128_t hash = math::FNVOFFSET128;
    auto len = StrLen(str);
    for (size_t i = 0; i < len; ++i) {
      hash ^= str[i];
      hash *= math::FNVPRIME128;
    }
    return hash;
  }

  uint128_t FNV1A128(const StringView &str) { return FNV1A128(str.c_str()); }


#if !defined(ROSE_NO_FORWARD_DECLARING_INTRINSICS)
  /**
   * @note Forward declaration to builtin intrinsic function. Do not delete.
   *          Clangd's indexer yells at you without this.
   */
  extern "C" uint64_t __rdtsc();
#endif

  struct FNV1A128State {
    uint128_t hash;
  };

  inline FNV1A128State FNV1A128Begin() noexcept {
    return {math::FNVOFFSET128};
  }

  inline void FNV1A128Update(FNV1A128State &s, const void *data, size_t len) noexcept {
    const auto *bytes = static_cast<const uint8_t *>(data);
    for (size_t i = 0; i < len; ++i) {
      s.hash ^= bytes[i];
      s.hash *= math::FNVPRIME128;
    }
  }

  inline uint128_t FNV1A128Finalize(const FNV1A128State &s) noexcept {
    return s.hash;
  }


  UUID UUID::Generate() noexcept {

    ++s_counter;
    // Returns the number of ticks (unit depends on the clock's period)
    uint64_t ticks = __rdtsc();

    auto tempcounter = s_counter + ticks;
    return UUID{FNV1A128(&tempcounter, sizeof(tempcounter))};
  }



  UUID UUID::Generate(const char *str) noexcept {

    const uint64_t counter = s_counter.fetch_add(1, std::memory_order_relaxed);
    const uint64_t ticks = __rdtsc();
    const size_t len = str ? StrLen(str) : 0;

    FNV1A128State state = FNV1A128Begin();
    FNV1A128Update(state, &counter, sizeof(counter));
    FNV1A128Update(state, &ticks, sizeof(ticks));
    if (len > 0) {
      FNV1A128Update(state, str, len);
    }
    return UUID{FNV1A128Finalize(state)};
  }
}
