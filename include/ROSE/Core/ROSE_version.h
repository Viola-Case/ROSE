/**

  @file      ROSE_version.h
  @brief     Compile-time and runtime version utilities
  @details   Provides the ROSE_VERSION_MAJOR/MINOR/PATCH defines, helper macros
             to pack and unpack the numeric version, and runtime functions to
             query the linked library version.
  @author    Viola Case
  @date      5.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_macros.h>
#include <ROSE/Core/ROSE_rtl.h>

#define ROSE_VERSION_MAJOR 0
#define ROSE_VERSION_MINOR 0
#define ROSE_VERSION_PATCH 2

/** @brief  Packs major, minor, and patch into a single unsigned int (major*1000000 + minor*10000 + patch). **/
constexpr unsigned int   ROSE_VERSIONNUM(char major, char minor, short patch) { return (1000000 * major + 10000 * minor + patch); }

/** @brief  Extracts the major component from a packed version number. **/
constexpr unsigned char  ROSE_VERSIONNUM_MAJOR(unsigned int version) { return (version / 1000000); }
/** @brief  Extracts the minor component from a packed version number. **/
constexpr unsigned char  ROSE_VERSIONNUM_MINOR(unsigned int version) { return ((version % 1000000) / 10000); }
/** @brief  Extracts the patch component from a packed version number. **/
constexpr unsigned short ROSE_VERSIONNUM_PATCH(unsigned int version) { return (version % 10000); }

/** @brief  Packed compile-time version number of this ROSE build. **/
constexpr unsigned int   ROSE_VERSION = ROSE_VERSIONNUM(ROSE_VERSION_MAJOR, ROSE_VERSION_MINOR, ROSE_VERSION_PATCH);

namespace ROSE {
  /**
    @brief   Returns the packed version number of the linked ROSE library.
    @retval  Version integer produced by ROSE_VERSIONNUM().
  **/
  unsigned int GetVersion();

  /**
    @brief   Formats a packed version number as a "major.minor.patch" string.
    @param   version  Packed version number (e.g. from GetVersion()).
    @retval  Human-readable version string.
  **/
  String VersionStr(unsigned int version);
}

//const char *Rose_Versionnum_Str(unsigned int version);

/** @brief  Compile-time "major.minor.patch" string literal for this build. **/
#define ROSE_VERSION_STR \
ROSE_XSTR(ROSE_VERSION_MAJOR) "." \
ROSE_XSTR(ROSE_VERSION_MINOR) "." \
ROSE_XSTR(ROSE_VERSION_PATCH)
