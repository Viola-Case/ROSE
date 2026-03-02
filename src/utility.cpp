#include <ROSE/ROSE.h>

namespace ROSE {
  void MemCpy(void *_Dst, const void *_Src, size_t size) {
    auto *p = reinterpret_cast<unsigned char *>(_Dst);
    auto *q = reinterpret_cast<const unsigned char *>(_Src);

    while (size--) {
      *p++ = *q++;
    }
  }
}