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

#if ROSE_PLATFORM_WINDOWS
  #include <Windows.h>
  #pragma comment(lib, "winmm.lib")
#endif

namespace ROSE {

  constexpr ApplicationFlags ROSE_APPLICATION_DEFAULT {
    APPLICATION_CONTROLLER_SUPPORT | APPLICATION_SOFTWARE_RENDERER
  };

  Application::Application(const char *_title, ApplicationFlags flags, List<Scene> &&scenes)
      : m_title(_title), m_scenes(Move(scenes)), m_flags(flags) {
#if ROSE_PLATFORM_WINDOWS
    timeBeginPeriod(1);
#endif
  }

  Application::Application(const char *_title, ApplicationFlags flags) : Application(_title, flags, {}) {}

  Application::Application(const char *_title) : Application(_title, APPLICATION_LIGHTWEIGHT) {}

  Application::Application() : Application("Game Title") {}

  int Application::Init() {
    if (SDL_VERSION != SDL_GetVersion()) {
      ROSE_LOG_FATAL("SDL API version and linked version mismatch!");
      return -2;
    }

    SDL_InitFlags initFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS;

    if (GetFlag(ApplicationFlag::ControllerSupport)) initFlags |= SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC;

    if (!SDL_Init(initFlags)) {
      ROSE_LOG_FATAL("SDL Init failed: ", SDL_GetError());
      return -3;
    }

    InputSystem::GetInstance().Init();

    SDL_WindowFlags windowFlags = SDL_WINDOW_HIDDEN;

    if (!GetFlag(ApplicationFlag::Headless)) {
      if (!GetFlag(ApplicationFlag::SoftwareRenderer)) {
        // figure out opengl vs vulkan here
        windowFlags |= (1 ? SDL_WINDOW_VULKAN : SDL_WINDOW_OPENGL);

        // maybe eventually figure out directx but maybe not
      } else {
        m_renderer = new SoftwareRenderer();
      }
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();

    m_window = SDL_CreateWindow(m_title.c_str(), m_windowSize.x, m_windowSize.y, windowFlags);

    RenderBackendContext ctx {
      m_window,
      m_windowSize.x,
      m_windowSize.y
    };

    m_renderer->Init(ctx);

    m_currentScene = m_scenes.begin();
    for (Scene &s : m_scenes)
      s.Bind(*this);


    return 0;
  }

  void Application::Run() {
    if (m_isRunning) return;

    SDL_ShowWindow(static_cast<SDL_Window *>(m_window));

    std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();


    m_isRunning = true;

    while (!m_shouldClose) {
      Scene &curScene = *m_currentScene;

      MemCpy(InputSystem::GetInstance().m_keyStatePrevious, InputSystem::GetInstance().m_keyState, 256);

      m_renderer->BeginFrame();
      ImGui::NewFrame();

      /* TODO this only tracks the previous scene, so switching back to one that already
       * ran replays OnCreate and OnStart over every behavior in it. Wants a per-scene
       * "started" flag rather than a pointer compare. Only bites once there are 2+ scenes.*/
      static Scene *lastScene { nullptr };
      if (lastScene != m_currentScene) {
        curScene.OnStart();
        lastScene = m_currentScene;
      }
      {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
          if (e.type == SDL_EVENT_QUIT) m_shouldClose = true;
          ImGui_ImplSDL3_ProcessEvent(&e);
        }
      }
      std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();
      auto dur = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(end - start);
      start = end;
      Time::dT = dur.count();
      curScene.FrameUpdate();

      ImGui::Render();

      m_renderer->EndFrame();

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    m_isRunning = false;

    m_renderer->Shutdown();

    SDL_DestroyWindow(static_cast<SDL_Window *>(m_window));
  }

  void Application::Quit() noexcept { m_shouldClose = true; }

  Application::~Application() {
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
      return;
    }
    SDL_Quit();
    timeEndPeriod(1);
    delete m_renderer;
  }

  const char *Application::GetTitle() const noexcept { return m_title.c_str(); }

  void *Application::GetWindow() const noexcept { return m_window; }

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

  // TODO: Make a SetInitialWindowSize function or something 
  void Application::SetWindowSize(const math::Vec2<int> size) noexcept {
    m_windowSize = size;
    if (m_window)
      SDL_SetWindowSize(static_cast<SDL_Window *>(m_window), m_windowSize.x, m_windowSize.y);
    else
      ROSE_LOG_WARN("No window to resize! Are you sure you've structured your application correctly?\n");
    if (m_renderer)
      m_renderer->OnResize(size.x, size.y);
  }

  void AddSceneFromJSON(void *jsonPtr) noexcept {}
} // namespace ROSE
