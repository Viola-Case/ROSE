/**

  @file       version.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       10.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.

 **/

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