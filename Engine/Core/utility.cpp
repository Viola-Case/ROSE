#include <ROSE/ROSE.h>

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

  __uint128_t FNV1A128(const void *data, size_t len) {
    __uint128_t hash = math::FNVOFFSET128;
    for (size_t i = 0; i < len; ++i) {
      hash ^= static_cast<const uint8_t *>(data)[i];
      hash *= math::FNVPRIME128;
    }
    return hash;
  }

  UUID UUID::Generate() noexcept {
    static uint64_t s_counter{0};
    ++s_counter;
    __uint128_t h = FNV1A128(&s_counter, sizeof(s_counter));
    return UUID{static_cast<uint64_t>(h >> 64), static_cast<uint64_t>(h)};
  }
}