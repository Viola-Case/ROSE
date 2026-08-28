/**

  @file      api.h
  @brief     DLL import/export annotations
  @details   ~
  @author    Viola Case
  @date      13.08.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/platform.h>
#include <ROSE/Core/macros.h>

#if ROSE_PLATFORM_WINDOWS
  #define ROSE_API_EXPORT __declspec(dllexport)
  #define ROSE_API_IMPORT __declspec(dllimport)
#else
  #define ROSE_API_EXPORT __attribute__((visibility("default")))
  #define ROSE_API_IMPORT
#endif

// Expands to ROSE_<MODULE>_API, e.g. ROSE_API(CORE) -> ROSE_CORE_API. Each module
// defines its own switch below off a build-time define that CMake sets PRIVATE on
// the module's target, so the module's own TUs export and everybody else imports.
#define ROSE_API(MODULE) ROSE_XCAT(ROSE_, ROSE_XCAT(MODULE, _API))

// The guard is an escape hatch for standalone builds that compile Core sources
// straight into an executable (see docs/internal/README.md): -DROSE_CORE_API=
// neutralizes the annotation instead of importing from a DLL that isn't there.
#if !defined(ROSE_CORE_API)
  #if defined(ROSE_CORE_BUILD)
    #define ROSE_CORE_API ROSE_API_EXPORT
  #else
    #define ROSE_CORE_API ROSE_API_IMPORT
  #endif
#endif

/*!
 * Modders should call this on shipped games so they can build on the correct baseline and address library
 * @return null-terminated string of the full
 */
extern "C" ROSE_API(CORE) const char *ROSE_GetBaseline();
