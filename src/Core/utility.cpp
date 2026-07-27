#include <ROSE/ROSE.h>

#include <chrono>
#include <intrin.h>


namespace ROSE {
  void MemCpy(void *_Dst, const void *_Src, size_t size) {
    auto *p = reinterpret_cast<unsigned char *>(_Dst);
    auto *q = reinterpret_cast<const unsigned char *>(_Src);

    while (size-- && reinterpret_cast<uintptr_t>(p) & 0x7) {
      *p++ = *q++;
    }

    constexpr size_t s = sizeof(uint64_t);

    while (size >= s) {
      size -= s;
      *reinterpret_cast<uint64_t *>(p) = *reinterpret_cast<const uint64_t *>(q);
      p+=s;
      q+=s;
    }
    while (size--)
      *p++ = *q++;
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
    return { math::FNVOFFSET128 };
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


  // this is a really dumb implementation but it's mine and i love it anyway
  UUID UUID::Generate() noexcept {
    /* Returns the number of ticks. This is the reason this generation system is extremely unlikely to result in
     * collisions.
     * @note Callers should probably be single-threaded: two cores reading the TSC at the same instant get the same
     * value, and there is no longer a tie-breaker to separate them. But that's really unlikely so whatever I guess.
     *
     */
    const uint64_t ticks = __rdtsc();

    return UUID { FNV1A128(&ticks, sizeof(ticks)) };
  }


} // namespace ROSE
