/**

    @file      runtime.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      27.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <ROSE/Core/string.h>

namespace ROSE {
  class RuntimeInfo {
  public:
    String GetOperatingSystem();
  };
}