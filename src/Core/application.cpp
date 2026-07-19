/**

  @file      application.cpp
  @brief
  @details   ~
  @author    Viola Case
  @date      9.03.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/

#include <thread>
#include <ROSE/ROSE.h>
#include <SDL3/SDL.h>
#include <ROSE/Core/time.h>
#include <nlohmann/json.hpp>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_impl_opengl3.h>

#if ROSE_PLATFORM_WINDOWS
#include <Windows.h>
#pragma comment(lib, "winmm.lib")
#endif

namespace ROSE {
  Application::Application(const char *_title, ApplicationFlags flags, List<Scene> &&scenes) : m_title(_title),
    m_scenes(Move(scenes)), m_flags(flags) {
    #if ROSE_PLATFORM_WINDOWS
    timeBeginPeriod(1);
    #endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
  }

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
    for (Scene &s : m_scenes) s.Bind(*this);

    return 0;
  }

  void Application::Run() {
    if (m_isRunning) return;
    m_isRunning = true;
    m_window = SDL_CreateWindow(m_title.c_str(), 800, 800, SDL_WINDOW_HIDDEN);
    SDL_ShowWindow(static_cast<SDL_Window *>(m_window));

    std::chrono::time_point<std::chrono::high_resolution_clock> start =
      std::chrono::high_resolution_clock::now();

    if (GetFlag(ApplicationFlag::SoftwareRenderer)) {
      m_renderer = SDL_CreateRenderer(static_cast<SDL_Window *>(m_window), nullptr);
      ImGui_ImplSDL3_InitForSDLRenderer(static_cast<SDL_Window *>(m_window), static_cast<SDL_Renderer *>(m_renderer));
      ImGui_ImplSDLRenderer3_Init(static_cast<SDL_Renderer *>(m_renderer));
    }

    Scene &curScene = *m_currentScene;

    while (!m_shouldClose) {

      if (GetFlag(ApplicationFlag::SoftwareRenderer)) {
        SDL_SetRenderDrawColor(static_cast<SDL_Renderer *>(m_renderer), 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(static_cast<SDL_Renderer *>(m_renderer));
      }
      ImGui::NewFrame();

      static Scene *lastScene{nullptr};
      if (lastScene != m_currentScene) {
        curScene.OnStart();
        lastScene = m_currentScene;
      }
      {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
          if (e.type == SDL_EVENT_QUIT) m_shouldClose = true;
        }
        ImGui_ImplSDL3_ProcessEvent(&e);
      }
      std::chrono::time_point<std::chrono::high_resolution_clock> end =
        std::chrono::high_resolution_clock::now();
      auto dur = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1,1>>>(end-start);
      start = end;
      Time::dT = dur.count();
      curScene.FrameUpdate();

      ImGui::Render();

      if (GetFlag(ApplicationFlag::SoftwareRenderer)) {
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), static_cast<SDL_Renderer *>(m_renderer));
        SDL_RenderPresent(static_cast<SDL_Renderer *>(m_renderer));
      }
      // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    SDL_DestroyWindow(static_cast<SDL_Window *>(m_window));
    m_isRunning = false;
  }

  void Application::Quit() noexcept {
    m_shouldClose = true;
  }

  Application::~Application() {
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
      return;
    }
    SDL_Quit();
    timeEndPeriod(1);
  }

  const char *Application::GetTitle() const noexcept {
    return m_title.c_str();
  }

  void *Application::GetWindow() const noexcept {
    return m_window;
  }

  const List<Scene> &Application::GetScenes() noexcept { return m_scenes; }
  Scene &Application::GetCurrentScene() noexcept { return *m_currentScene; }

  void Application::SetFlag(ApplicationFlag m, bool b) noexcept {
    const auto mask = 1 << m;
    if (b) m_flags |= mask;
    else m_flags &= ~mask;
  }

  bool Application::GetFlag(ApplicationFlag m) const noexcept {
    const auto mask = 1 << m;
    return m_flags & mask;
  }

  void Application::SetWindowSize(const math::Vec2<int> size) noexcept {
    m_windowSize = size;
  }



  void AddSceneFromJSON(void *jsonPtr) noexcept {

  }
}
