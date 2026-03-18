#include <ROSE/ROSE.h>

namespace ROSE {
  void MemCpy(void *_Dst, const void *_Src, size_t size) {
    auto *p = reinterpret_cast<unsigned char *>(_Dst);
    auto *q = reinterpret_cast<const unsigned char *>(_Src);

    while (size--) {
      *p++ = *q++;
    }
  }


  uint64_t FNV1A(const void *data, size_t len) {
    uint64_t hash = math::FNVOFFSET64;
    for (int i = 0; i < len; ++i) {
      hash ^= static_cast<const char8_t *>(data)[i];
      hash *= math::FNVPRIME64;
    }
    return hash;
  }
  uint64_t FNV1A(const char8_t *str) {
    uint64_t hash = math::FNVOFFSET64;
    auto len = StrLen(str);
    for (int i = 0; i < len; ++i) {
      hash ^= str[i];
      hash *= math::FNVPRIME64;
    }
    return hash;
  }
  uint64_t FNV1A(const char16_t *str) {
    uint64_t hash = math::FNVOFFSET64;
    auto len = StrLen(str);
    for (int i = 0; i < len; ++i) {
      hash ^= (str[i] & 0xFF);
      hash *= math::FNVPRIME64;
      hash ^= (str[i] >> 8) & 0xFF;
      hash *= math::FNVPRIME64;
    }
    return hash;
  }

  uint64_t FNV1A(const String &str) { return FNV1A(str.c_str()); }
  uint64_t FNV1A(const WString &str) { return FNV1A(str.c_str()); }
}