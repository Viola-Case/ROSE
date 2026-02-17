/**

  @file      ROSE_version.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      5.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/ROSE_preamble.h>

#define ROSE_VERSION_MAJOR 0
#define ROSE_VERSION_MINOR 0
#define ROSE_VERSION_PATCH 1

constexpr unsigned int   ROSE_VERSIONNUM(char major, char minor, short patch) { return (1000000 * major + 10000 * minor + patch); }

constexpr unsigned char  ROSE_VERSIONNUM_MAJOR(unsigned int version) { return (version / 1000000); }
constexpr unsigned char  ROSE_VERSIONNUM_MINOR(unsigned int version) { return ((version % 1000000) / 10000); }
constexpr unsigned short ROSE_VERSIONNUM_PATCH(unsigned int version) { return (version % 10000); }

constexpr unsigned int   ROSE_VERSION = ROSE_VERSIONNUM(ROSE_VERSION_MAJOR, ROSE_VERSION_MINOR, ROSE_VERSION_PATCH);

namespace ROSE { unsigned int GetVersion(); }

//const char *Rose_Versionnum_Str(unsigned int version);

#define ROSE_VERSION_STR \
ROSE_XSTR(ROSE_VERSION_MAJOR) "." \
ROSE_XSTR(ROSE_VERSION_MINOR) "." \
ROSE_XSTR(ROSE_VERSION_PATCH)
