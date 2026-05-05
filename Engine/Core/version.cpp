/**
 *
 *//

#include <ROSE/ROSE.h>

namespace ROSE {
  unsigned int GetVersion() { return ROSE_VERSION; }
  String VersionStr(unsigned int v) {
    return Format("{}.{}.{}",
      ROSE_VERSIONNUM_MAJOR(v),
      ROSE_VERSIONNUM_MINOR(v),
      ROSE_VERSIONNUM_PATCH(v)
    );
  }
}