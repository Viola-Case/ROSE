/**

  @file      ROSE_preamble.h
  @brief     Preamble at the beginning of every public header in the ROSE library
  @details   ~
  @author    Viola Case
  @date      4.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <cstdlib>
#include <cstdint>
#include <type_traits>
#include <utility>

#if defined (_MSC_VER)
#include <intrin.h>
#endif

#include <ROSE/Core/ROSE_stdlib.h>
#include <ROSE/Core/ROSE_macros.h>
#include <ROSE/Core/ROSE_rtl.h>
#include <ROSE/Core/ROSE_typetraits.h>
#include <ROSE/Core/ROSE_uuid.h>

namespace rose = ROSE;