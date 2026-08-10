/**

  @file      version.h
  @brief
  @details   ~
  @author    Viola Case
  @date      5.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/macros.h>
#include <ROSE/Core/rtl.h>

// so anyway I stole basically all of this from SDL

#define ROSE_VERSION_MAJOR 0
#define ROSE_VERSION_MINOR 1
#define ROSE_VERSION_PATCH 5

constexpr unsigned int ROSE_VERSIONNUM(char major, char minor, short patch) { return (1000000 * major + 10000 * minor + patch); }

constexpr unsigned char ROSE_VERSIONNUM_MAJOR(unsigned int version) { return (version / 1000000); }
constexpr unsigned char ROSE_VERSIONNUM_MINOR(unsigned int version) { return ((version % 1000000) / 10000); }
constexpr unsigned short ROSE_VERSIONNUM_PATCH(unsigned int version) { return (version % 10000); }

constexpr unsigned int ROSE_VERSION = ROSE_VERSIONNUM(ROSE_VERSION_MAJOR, ROSE_VERSION_MINOR, ROSE_VERSION_PATCH);

namespace ROSE {
  ROSE_API(Core) unsigned int GetVersion();

  ROSE_API(Core) String VersionStr(unsigned int version);

  ROSE_API(Core) bool DebugCheckVersion(unsigned int version); //!< This is called by ROSE_CHECK_VERSION() macro
} // namespace ROSE

// const char *Rose_Versionnum_Str(unsigned int version);

#define ROSE_VERSION_STR        \
  ROSE_XSTR(ROSE_VERSION_MAJOR) \
  "." ROSE_XSTR(ROSE_VERSION_MINOR) "." ROSE_XSTR(ROSE_VERSION_PATCH)



#define ROSE_CHECK_VERSION() ROSE::DebugCheckVersion(ROSE_VERSION)
