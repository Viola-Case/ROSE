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
#include <nlohmann/json.hpp>

namespace ROSE {
  Application::Application(const char *_title, ApplicationFlags flags, List<Scene> &&scenes) : m_title(_title),
    m_flags(flags), m_scenes(Move(scenes)) {}

  Application::Application(const char *_title, ApplicationFlags flags) : Application(_title, flags, {}) {}

  Application::Application(const char *_title) : Application(_title, APPLICATION_LIGHTWEIGHT) {}

  Application::Application() : Application("Game Title") {}

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

    m_currentScene = m_scenes.begin();

    return 0;
  }

  void Application::Run() {
    while (!m_shouldClose) {
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) m_shouldClose = true;
      }
      Scene &curScene = *m_currentScene;
      curScene.FrameUpdate();
    }
  }

  void Application::Quit() noexcept {
    m_shouldClose = true;
  }

  Application::~Application() {
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
      return;
    }
    SDL_Quit();
  }

  const char *Application::GetTitle() const noexcept {
    return m_title.c_str();
  }

  void *Application::GetWindow() const noexcept {
    return m_window;
  }

  List<Scene> &Application::GetScenes() noexcept { return m_scenes; }
  Scene &Application::GetCurrentScene() noexcept { return *m_currentScene; }


  void AddSceneFromJSON(void *jsonPtr) noexcept {

  }
}
