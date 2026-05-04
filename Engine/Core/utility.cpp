#include <ROSE/ROSE.h>

#include <chrono>
#include <intrin.h>

//#pragma intrinsic(__rdtsc)

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

  UUID UUID::Generate() noexcept {
    static uint64_t s_counter{0};

    ++s_counter;
    // Returns the number of ticks (unit depends on the clock's period)
    uint64_t ticks = __rdtsc(); // NOLINT

    auto tempcounter = s_counter + ticks;
    return UUID{FNV1A128(&tempcounter, sizeof(tempcounter))};
  }

  UUID UUID::Generate(char *str) noexcept {
    static uint64_t s_counter{0};

    ++s_counter;
    // Returns the number of ticks (unit depends on the clock's period)
    long long ticks = __rdtsc(); // NOLINT

    auto tempcounter = s_counter + ticks;
    auto len = StrLen(str);
    char *ptr;
    char *ptrOther;
    if (len >= sizeof(tempcounter)) {
      ptr = str;
      ptrOther = reinterpret_cast<char*>(&tempcounter);
    } else {
      ptr = reinterpret_cast<char *>(&tempcounter);
      ptrOther = str;
    }
    for (size_t i = 0; i < len; ++i) {

    }
    return UUID{FNV1A128(ptr, Max(len,sizeof(tempcounter)))};
  }
}
