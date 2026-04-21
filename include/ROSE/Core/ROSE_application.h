/**

  @file      ROSE_application.h
  @brief     Application class and startup flag definitions
  @details   Application is the top-level container managing the window,
             renderer, and scene list. Construct it with a title and optional
             flags, populate the scene list, then call Init() and Run() to
             enter the main loop.
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
  constexpr ApplicationFlags APPLICATION_HEADLESS          = 1 << 0; //!< Disables window creation; runs without a display
  constexpr ApplicationFlags APPLICATION_NO_RENDERER       = 1 << 1; //!< Skips renderer initialisation; window is still created
  constexpr ApplicationFlags APPLICATION_SERVER            = 1 << 2; //!< Headless + server-oriented initialisation
  constexpr ApplicationFlags APPLICATION_LIGHTWEIGHT       = 1 << 4; //!< Disables non-essential subsystems to reduce overhead
  constexpr ApplicationFlags APPLICATION_SOFTWARE_RENDERER = 1 << 5; //!< Forces CPU-side software rendering

  //constexpr ApplicationFlags AAA_GAME = APPLICATION_DIRECTX
  //
  //
  //
  //
  // class Application;
  //
  // class ApplicationManager {
  // public:
  //   static void Close(AppID);
  //   static void CloseAll();
  //   static Application &GetApplication(AppID idx = 0);
  //   static void LinkApplication(Application &app);
  // private:
  //   static TypedHashMap<AppID, Application *> m_applications;
  // };

  class SceneManager;

  /**
    @class   Application
    @brief   Top-level engine container that owns the window, renderer, and scenes.
    @details Manages the main loop, input system, and an ordered list of scenes.
             Exactly one scene is active at a time and is updated every frame.
             Typical usage:
             @code
             ROSE::Application app("My Game");
             if (app.Init() == 0)
               app.Run();
             @endcode
  **/
  class Application {
    friend class ApplicationManager;
  public:
    Application();                                                        //!< Default constructor; uses title "game" with no flags
    Application(const char *_title);                                      //!< Constructs with the given window title and no flags
    Application(const char *_title, ApplicationFlags);                    //!< Constructs with a title and startup flags
    Application(const char *_title, ApplicationFlags, List<Scene> &&);   //!< Constructs with a title, flags, and a pre-built scene list
    ~Application();

    /**
      @brief   Initialises the window, renderer, and all engine subsystems.
      @return  0 on success, non-zero on failure.
    **/
    int Init();

    /**
      @brief   Enters the blocking game loop.
      @details Returns only after Quit() has been called or the window is closed.
               Processes events, polls input, and calls FrameUpdate() on the
               active scene each iteration.
    **/
    void Run();

    /**
      @brief   Signals the main loop to stop after the current frame finishes.
    **/
    void Quit() noexcept;

    /**
      @brief   Releases all engine subsystems and frees resources.
      @details Invoked automatically by the destructor. Safe to call manually
               if early cleanup is required before destruction.
    **/
    void CleanUp();

    /**
      @brief   Returns the window title set at construction time.
      @retval  Null-terminated title string; lifetime is tied to this Application.
    **/
    [[nodiscard]] const char *GetTitle() const noexcept;

    /**
      @brief   Returns the underlying platform window handle.
      @retval  Opaque pointer to the SDL_Window; cast to SDL_Window* as needed.
    **/
    [[nodiscard]] void *GetWindow() const noexcept;

    /**
      @brief   Returns a reference to the currently active scene.
    **/
    Scene &GetCurrentScene() noexcept;

    /**
      @brief   Returns the full list of scenes owned by this application.
    **/
    List<Scene> &GetScenes() noexcept;

  private:
    String m_title{"game"};
    ApplicationManager *m_parent{nullptr};

    List<Scene> m_scenes{};
    Scene *m_currentScene{nullptr};

    AppID m_id{0};

    void *m_window{nullptr};

    ApplicationFlags m_flags{0};

    void *m_renderer{nullptr};

    UniquePtr<SceneManager> m_manager {};

    bool m_shouldClose{false};



  };
}
