/**

  @file      application.cpp
  @brief     
  @details   ~
  @author    Viola Case
  @date      9.03.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>
#include <SDL3/SDL.h>

namespace ROSE {
  int Application::Init() {
    if (ROSE_VERSION != ROSE::GetVersion()) {
      ROSE_LOG_FATAL("ROSE API version and linked version mismatch!");
      return -1;
    }
    if (SDL_VERSION != SDL_GetVersion()) {
      ROSE_LOG_FATAL("SDL API version and linked version mismatch!");
      return -2;
    }
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC)) {
      ROSE_LOG_FATAL("SDL Init failed: ", SDL_GetError());
      return -3;
    }
    InputSystem::GetInstance().Init();
  }

  void Application::Run() {
    while (true) {

    }
  }
}