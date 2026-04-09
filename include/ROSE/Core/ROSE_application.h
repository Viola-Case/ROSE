/**

  @file      ROSE_application.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      9.03.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <cstdlib>
#include <cstdint>
#include <ROSE/Core/ROSE_rtl.h>
#include <ROSE/Core/ROSE_scene.h>

namespace ROSE {
  using AppID = size_t;

  using ApplicationFlags = uint64_t;
  constexpr ApplicationFlags APPLICATION_HEADLESS     = 1 << 0;
  constexpr ApplicationFlags APPLICATION_NO_RENDERER  = 1 << 1;
  constexpr ApplicationFlags APPLICATION_SERVER       = 1 << 2;
  constexpr ApplicationFlags APPLICATION_LIGHTWEIGHT  = 1 << 4;
  constexpr ApplicationFlags APPLICATION_SOFTWARE_RENDERER = 1 << 5;

  //constexpr ApplicationFlags AAA_GAME = APPLICATION_DIRECTX




  class Application;

  class ApplicationManager {
  public:
    static void Close(AppID);
    static void CloseAll();
    static Application &GetApplication(AppID idx = 0);
    static void LinkApplication(Application &app);
  private:
    static TypedHashMap<AppID, Application *> m_applications;
  };

  class Application {
    friend class ApplicationManager;
  public:
    Application();
    Application(const char *_title);
    Application(const char *_title, ApplicationFlags);
    ~Application();
    int Init();
    void Run();
    void Quit() noexcept;
    void CleanUp();

    const char *GetTitle() const noexcept;

    void *GetWindow() const noexcept;

  private:
    String m_title{"game"};
    ApplicationManager *m_parent{nullptr};

    List<Scene> m_scenes{};

    AppID m_id{0};

    void *m_window{nullptr};

    ApplicationFlags m_flags{0};

    void *m_renderer{nullptr};

    bool m_shouldClose{false};
  };
}