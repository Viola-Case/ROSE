/**

  @file      application.cpp
  @brief
  @details   ~
  @author    Viola Case
  @date      9.03.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/

#include <fstream>
#include <sstream>
#include <thread>
#include <ROSE/ROSE.h>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <ROSE/Core/imgui.h>

#if ROSE_PLATFORM_WINDOWS
  #include <Windows.h>
  #pragma comment(lib, "winmm.lib")
#endif

namespace ROSE {

  /* Time's storage lives here rather than in the header so there is exactly one
  copy, inside ROSE_Core.dll. Application::Run() is the only writer. */
  double Time::dT {};
  const double &Time::deltaTime { Time::dT };

  //=========================Backing for ROSE::AttachImGui()=========================//
  //                    See ROSE/Core/imgui.h for why this exists
  void *GetImGuiContext() noexcept { return ImGui::GetCurrentContext(); }

  void GetImGuiAllocatorFunctions(void **allocFn, void **freeFn, void **userData) noexcept {
    ImGuiMemAllocFunc alloc {};
    ImGuiMemFreeFunc free {};
    ImGui::GetAllocatorFunctions(&alloc, &free, userData);
    *allocFn = reinterpret_cast<void *>(alloc);
    *freeFn = reinterpret_cast<void *>(free);
  }
  //=================================================================================//

#pragma region ApplicationInitSettings

  ApplicationInitSettings::ApplicationInitSettings() noexcept = default;
  ApplicationInitSettings::ApplicationInitSettings(const StringView _title) : m_title(_title) {}
  ApplicationInitSettings::~ApplicationInitSettings() = default;

  ApplicationInitSettings::ApplicationInitSettings(ApplicationInitSettings &&) noexcept = default;
  ApplicationInitSettings &ApplicationInitSettings::operator=(ApplicationInitSettings &&) noexcept = default;

  ApplicationInitSettings &ApplicationInitSettings::SetTitle(const StringView _title) noexcept {
    m_title = _title;
    return *this;
  }

  ApplicationInitSettings &ApplicationInitSettings::SetOrganization(const StringView _organization) noexcept {
    m_organization = _organization;
    return *this;
  }

  ApplicationInitSettings &ApplicationInitSettings::SetWindowSize(const math::Vec2<int> _size) noexcept {
    m_windowSize = _size;
    return *this;
  }

  ApplicationInitSettings &ApplicationInitSettings::SetFlags(const ApplicationFlags _flags) noexcept {
    m_flags = _flags;
    return *this;
  }

  ApplicationInitSettings &ApplicationInitSettings::SetFlag(const ApplicationFlag _flag, const bool _on) noexcept {
    const ApplicationFlags mask = static_cast<ApplicationFlags>(1) << _flag;
    if (_on) m_flags |= mask;
    else m_flags &= ~mask;
    return *this;
  }

  ApplicationInitSettings &ApplicationInitSettings::SetVSync(const bool _vsync) noexcept {
    m_vsync = _vsync;
    return *this;
  }

  ApplicationInitSettings &ApplicationInitSettings::SetTargetFrameRate(const uint32_t _fps) noexcept {
    m_targetFrameRate = _fps;
    return *this;
  }

  ApplicationInitSettings &ApplicationInitSettings::SetExistingWindow(void *_handle) noexcept {
    m_windowHandle = _handle;
    return *this;
  }

  ApplicationInitSettings &ApplicationInitSettings::AddScene(Scene &&_scene) noexcept {
    m_scenes.push_back(Move(_scene));
    return *this;
  }

  ApplicationInitSettings &ApplicationInitSettings::SetScenes(List<Scene> &&_scenes) noexcept {
    m_scenes = Move(_scenes);
    return *this;
  }

  ApplicationInitSettings &ApplicationInitSettings::AddSceneFromFile(const StringView _path) noexcept {
    /* A view is not required to be NUL-terminated, so go through a String for the C-facing
     * open() rather than handing it _path.c_str(). */
    const String path { _path };

    std::ifstream file(path.c_str());
    if (!file) {
      ROSE_LOG_ERROR("Scene file '{}' could not be opened.\n", path);
      return *this;
    }

    std::stringstream contents;
    contents << file.rdbuf();

    m_scenes.push_back(Scene::FromJSONString(String(contents.str())));
    return *this;
  }

#pragma endregion

  Application::Application() noexcept {
#if ROSE_PLATFORM_WINDOWS
    if (!GetFlag(ApplicationFlag::LowPerformance)) timeBeginPeriod(1);
#endif
  }

  int Application::Init(ApplicationInitSettings &&settings) {
    m_title = Move(settings.m_title);
    m_organization = Move(settings.m_organization);
    m_windowSize = settings.m_windowSize;
    m_flags = settings.m_flags;
    m_targetFrameRate = settings.m_targetFrameRate;
    m_vsync = settings.m_vsync;
    m_scenes = Move(settings.m_scenes);

#if defined(_DEBUG)
    m_flags |= APPLICATION_DEBUG;
#endif

    if (settings.m_windowHandle)
      ROSE_LOG_WARN("An existing window handle was supplied, but adopting one is not implemented yet - creating our "
                    "own instead.\n");

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

    if (!GetFlag(ApplicationFlag::Headless)) {
      m_window = MakeUnique<Window>(Window::Create(m_title.c_str(), m_windowSize, windowFlags));
      if (!m_window->IsValid()) {
        ROSE_LOG_FATAL("Window creation failed!\n");
        return -4;
      }
    }

    if (m_renderer && m_window) {
      const math::Vec2<int> size = m_window->GetSize();

      RenderBackendContext ctx { { m_window->GetHandle() }, size.x, size.y, m_vsync };

      m_renderer->Init(ctx);
    }

    /* An empty scene list is legal - a headless or server run may have nothing to update - so
     * this stays null rather than pointing at an empty List's storage, and `Run` checks it. */
    m_currentScene = m_scenes.empty() ? nullptr : m_scenes.begin();
    for (Scene &s : m_scenes)
      s.Bind(*this);


    return 0;
  }

  void Application::Run() {
    if (m_isRunning) return;

    if (m_window) m_window->Show();

    std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();


    m_isRunning = true;

    while (!m_shouldClose) {
      MemCpy(InputSystem::GetInstance().m_keyStatePrevious, InputSystem::GetInstance().m_keyState, 256);

      if (m_renderer) m_renderer->BeginFrame();
      ImGui::NewFrame();

      /* TODO this only tracks the previous scene, so switching back to one that already
       * ran replays OnCreate and OnStart over every behavior in it. Wants a per-scene
       * "started" flag rather than a pointer compare. Only bites once there are 2+ scenes.*/
      static Scene *lastScene { nullptr };
      if (m_currentScene && lastScene != m_currentScene) {
        m_currentScene->OnStart();
        lastScene = m_currentScene;
      }
      {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
          if (e.type == SDL_EVENT_QUIT) {
            m_shouldClose = true;
          } else if (e.type == SDL_EVENT_WINDOW_RESIZED) {
            /* The window only ever learns its new size from here; pushing it back to SDL would
             * just provoke another resize event. */
            const math::Vec2<int> size { e.window.data1, e.window.data2 };
            if (m_window) m_window->OnResized(size);
            if (m_renderer) m_renderer->OnResize(size.x, size.y);
          } else if (e.type == SDL_EVENT_WINDOW_MOVED) {
            if (m_window) m_window->OnMoved({ e.window.data1, e.window.data2 });
          }
          ImGui_ImplSDL3_ProcessEvent(&e);
        }
      }
      std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();
      auto dur = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(end - start);
      start = end;
      Time::dT = dur.count();
      if (m_currentScene) m_currentScene->FrameUpdate();

      ImGui::Render();

      if (m_renderer) m_renderer->EndFrame();

      /* `start` is this frame's beginning, so the elapsed time below covers everything the frame
       * did. Uncapped still gives up a millisecond rather than spinning a core flat. */
      if (const uint32_t fps = m_targetFrameRate) {
        const std::chrono::duration<double> budget { 1.0 / static_cast<double>(fps) };
        const std::chrono::duration<double> elapsed { std::chrono::high_resolution_clock::now() - start };
        if (elapsed < budget) std::this_thread::sleep_for(budget - elapsed);
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
    m_isRunning = false;

    if (m_renderer) m_renderer->Shutdown();
  }

  void Application::Quit() noexcept { m_shouldClose = true; }

  Application::~Application() {
    /* Order matters: the renderer holds resources made from the window, and the window has to be
     * gone before SDL_Quit. Resetting m_window here rather than leaving it to member destruction
     * is deliberate - members die after this body, which would be after SDL_Quit. */
    delete m_renderer;
    m_renderer = nullptr;

    m_window.reset();

    if (SDL_WasInit(SDL_INIT_VIDEO)) SDL_Quit();
#if ROSE_PLATFORM_WINDOWS
    timeEndPeriod(1);
#endif
  }

  const char *Application::GetTitle() const noexcept { return m_title.c_str(); }
  const char *Application::GetOrganization() const noexcept { return m_organization.c_str(); }

  Window *Application::GetWindow() noexcept { return m_window.get(); }
  const Window *Application::GetWindow() const noexcept { return m_window.get(); }

  const List<Scene> &Application::GetScenes() noexcept { return m_scenes; }
  Scene *Application::GetCurrentScene() noexcept { return m_currentScene; }

  bool Application::GetFlag(ApplicationFlag m) const noexcept {
    const ApplicationFlags mask = static_cast<ApplicationFlags>(1) << m;
    return m_flags & mask;
  }

  uint32_t Application::GetTargetFrameRate() const noexcept { return m_targetFrameRate; }
  void Application::SetTargetFrameRate(const uint32_t fps) noexcept { m_targetFrameRate = fps; }

  void Application::SetWindowSize(const math::Vec2<int> size) noexcept {
    m_windowSize = size;
    if (m_window) m_window->SetSize(size);
    else ROSE_LOG_WARN("No window to resize! Are you sure you've structured your application correctly?\n");
    if (m_renderer) m_renderer->OnResize(size.x, size.y);
  }
} // namespace ROSE
