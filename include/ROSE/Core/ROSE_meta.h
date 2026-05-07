/**

    @file      ROSE_meta.h
    @brief
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/preamble/ROSE_stdlib.h>

namespace ROSE {
  struct MetaInfo {
    char FileName[256];
    char RelativePath[256];
  };
} // namespace ROSE