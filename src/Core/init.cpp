/**

  @file       init.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       18.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>
#include <SDL3/SDL.h>

namespace ROSE {
  InitStatus Init() {
    if (SDL_VERSION != SDL_GetVersion()) {
      ROSE_LOG_FATAL("SDL API version and linked version mismatch!");
      return InitStatus::SDLVersionMismatch;
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK |
                  SDL_INIT_HAPTIC)) {
      ROSE_LOG_FATAL("SDL Init failed: ", SDL_GetError());
      return InitStatus::SDLInitFailed;
    }

    InputSystem::GetInstance().Init();

    return InitStatus::Success;
  }
} // namespace ROSE