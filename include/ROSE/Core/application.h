/**

  @file      application.h
  @brief
  @details   ~
  @author    Viola Case
  @date      9.03.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <cstdlib>
#include <cstdint>
#include <ROSE/Core/rtl.h>
#include <ROSE/Core/factory.h>

namespace ROSE {
  //using AppID = size_t;

  /*!
   * Not to be confused with `ApplicationFlags`
   */
  struct ApplicationFlag {
    enum Value : uint8_t {
      Headless = 0,
      NoRenderer = 1,
      Server = 2,
      Light = 3,
      SoftwareRenderer = 4,
      RichPresence = 5,
    } value;
    constexpr ApplicationFlag(Value v) noexcept : value(v <= 63 ? v : throw "invalid ApplicationFlag") {}
    //constexpr ApplicationFlag(uint8_t v) noexcept : value(static_cast<Value>(v)) {}
    constexpr operator uint64_t() const noexcept { return value; }
  };

  /*!
   * Not to be confused with `ApplicationFlag`
   */
  using ApplicationFlags = uint64_t;
  constexpr ApplicationFlags APPLICATION_HEADLESS = static_cast<ApplicationFlags>(1) << static_cast<int>(ApplicationFlag::Headless);
  constexpr ApplicationFlags APPLICATION_NO_RENDERER = static_cast<ApplicationFlags>(1) << static_cast<int>(ApplicationFlag::NoRenderer);
  constexpr ApplicationFlags APPLICATION_SERVER = static_cast<ApplicationFlags>(1) << static_cast<int>(ApplicationFlag::Server);
  constexpr ApplicationFlags APPLICATION_LIGHTWEIGHT = static_cast<ApplicationFlags>(1) << static_cast<int>(ApplicationFlag::Light);
  constexpr ApplicationFlags APPLICATION_SOFTWARE_RENDERER = static_cast<ApplicationFlags>(1) << static_cast<int>(ApplicationFlag::SoftwareRenderer);
  constexpr ApplicationFlags APPLICATION_RICHPRESENCE = static_cast<ApplicationFlags>(1) << static_cast<int>(ApplicationFlag::RichPresence);



  class Scene;
  // constexpr ApplicationFlags AAA_GAME = APPLICATION_DIRECTX
  //
  //
  //
  //
  //  class Application;
  //
  //  class ApplicationManager {
  //  public:
  //    static void Close(AppID);
  //    static void CloseAll();
  //    static Application &GetApplication(AppID idx = 0);
  //    static void LinkApplication(Application &app);
  //  private:
  //    static TypedHashMap<AppID, Application *> m_applications;
  //  };

  class SceneManager;



  class Application final {
    /**
     * @todo probably just ditch this one
     */
    friend class ApplicationManager;

  public:
    Application();
    Application(const char *_title);
    Application(const char *_title, ApplicationFlags);
    Application(const char *_title, ApplicationFlags, List<Scene> &&);
    ~Application();
    int Init();
    void Run();
    void Quit() noexcept;
    void CleanUp();

    [[nodiscard]] const char *GetTitle() const noexcept;

    [[nodiscard]] void *GetWindow() const noexcept;

    Scene &GetCurrentScene() noexcept;

    List<Scene> &GetScenes() noexcept;



    void SetFlag(ApplicationFlag, bool) noexcept;

  private:
    String m_title { "game" };
    ApplicationManager *m_parent { nullptr };

    List<Scene> m_scenes {};
    Scene *m_currentScene { nullptr };

    //AppID m_id { 0 };

    void *m_window { nullptr };

    ApplicationFlags m_flags { 0 };

    void *m_renderer { nullptr };

    UniquePtr<SceneManager> m_manager {};

    bool m_shouldClose { false };
  };
} // namespace ROSE