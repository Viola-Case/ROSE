/**

    @file      meta.h
    @brief
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.
    @deprecated yea i think this file is completely deprecated and should be marked for removal

**/
#pragma once

#include <ROSE/Core/stdlib.h>

namespace ROSE {
  struct MetaInfo {
    char FileName[256];
    char RelativePath[256];
  };
} // namespace ROSE