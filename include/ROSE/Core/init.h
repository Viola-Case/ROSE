/**

  @file       init.h
  @brief
  @details    ~
  @author     Viola Case
  @date       18.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <cstdint>

#include <ROSE/Core/api.h>

namespace ROSE {

  enum class InitStatus : uint8_t {
    Success = 0,
    UnknownFailure,
    SDLVersionMismatch,
    SDLInitFailed,

  };

  ROSE_API(Core) InitStatus Init();
} // namespace ROSE