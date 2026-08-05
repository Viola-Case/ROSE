/**

  @file      application.h
  @brief
  @details   ~
  @author    Viola Case
  @date      9.03.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <cstdint>
#include <ROSE/Core/rtl.h>
#include <ROSE/Core/factory.h>
#include <ROSE/Core/gfx.h>
#include <ROSE/Core/window.h>

namespace ROSE {
  // using AppID = size_t;

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
      ControllerSupport = 5,
      NoSound = 6,
      Vulkan = 7,
      OpenGL = 8,
      DirectX9 = 9,
      Metal = 10,
      DirectX11 = 11,
      DirectX12 = 12,

      Debug = 63
    } value;
    constexpr ApplicationFlag(Value v) : value(v <= 63 ? v : throw "invalid ApplicationFlag") {}
    // constexpr ApplicationFlag(uint8_t v) noexcept : value(static_cast<Value>(v)) {}
    constexpr operator uint64_t() const noexcept { return value; }
  };

  /*!
   * Not to be confused with `ApplicationFlag`
   */
  using ApplicationFlags = uint64_t;
  constexpr ApplicationFlags APPLICATION_HEADLESS = static_cast<ApplicationFlags>(1)
                                                    << static_cast<int>(ApplicationFlag::Headless);
  constexpr ApplicationFlags APPLICATION_NO_RENDERER = static_cast<ApplicationFlags>(1)
                                                       << static_cast<int>(ApplicationFlag::NoRenderer);
  constexpr ApplicationFlags APPLICATION_SERVER = static_cast<ApplicationFlags>(1)
                                                  << static_cast<int>(ApplicationFlag::Server);
  constexpr ApplicationFlags APPLICATION_LIGHTWEIGHT = static_cast<ApplicationFlags>(1)
                                                       << static_cast<int>(ApplicationFlag::Light);
  constexpr ApplicationFlags APPLICATION_SOFTWARE_RENDERER = static_cast<ApplicationFlags>(1)
                                                             << static_cast<int>(ApplicationFlag::SoftwareRenderer);
  constexpr ApplicationFlags APPLICATION_CONTROLLER_SUPPORT = static_cast<ApplicationFlags>(1)
                                                              << static_cast<int>(ApplicationFlag::ControllerSupport);
  constexpr ApplicationFlags APPLICATION_VULKAN = static_cast<ApplicationFlags>(1)
                                                  << static_cast<int>(ApplicationFlag::Vulkan);
  constexpr ApplicationFlags APPLICATION_DIRECTX9 = static_cast<ApplicationFlags>(1)
                                                    << static_cast<int>(ApplicationFlag::DirectX9);
  constexpr ApplicationFlags APPLICATION_METAL = static_cast<ApplicationFlags>(1)
                                                 << static_cast<int>(ApplicationFlag::Metal);
  constexpr ApplicationFlags APPLICATION_DIRECTX11 = static_cast<ApplicationFlags>(1)
                                                     << static_cast<int>(ApplicationFlag::DirectX11);
  constexpr ApplicationFlags APPLICATION_DIRECTX12 = static_cast<ApplicationFlags>(1)
                                                     << static_cast<int>(ApplicationFlag::DirectX12);

  constexpr ApplicationFlags APPLICATION_DEBUG = static_cast<ApplicationFlags>(1)
                                                 << static_cast<int>(ApplicationFlag::Debug);





  class Scene;


  class SceneManager;

  class Application final {

  public:
    Application();
    Application(const char *_title);
    Application(const char *_title, ApplicationFlags);
    Application(const char *_title, ApplicationFlags, List<Scene> &&, math::Vec2<int> _windowSize = { 800, 600 });
    ~Application();
    int Init();
    void Run(); //!< Should only be called once. Terminates if called again.
    void Quit() noexcept;
    // void CleanUp();

    [[nodiscard]] const char *GetTitle() const noexcept;

    /*!
     * @retval the owned window, or `nullptr` when the application runs headless. Always check
     *         before dereferencing - a headless run legitimately has no window.
     */
    [[nodiscard]] Window *GetWindow() noexcept;
    [[nodiscard]] const Window *GetWindow() const noexcept;

    Scene &GetCurrentScene() noexcept;

    const List<Scene> &GetScenes() noexcept;

    void SetFlag(ApplicationFlag, bool) noexcept;

    bool GetFlag(ApplicationFlag) const noexcept;

    void SetWindowSize(math::Vec2<int> size) noexcept;
    inline void SetWindowSize(int width, int height) noexcept { SetWindowSize({ width, height }); }

    bool LoadModule(StringView name) noexcept;

  protected:
    String m_title { "game" };

    List<Scene> m_scenes {};
    Scene *m_currentScene { nullptr };

    // AppID m_id { 0 };

    UniquePtr<Window> m_window {};

    ApplicationFlags m_flags { 0 };

    RenderBackend *m_renderer { nullptr };

    UniquePtr<SceneManager> m_manager {};

    /* The size the window is created at, taken from the constructor. Authoritative only until
     * `Init` builds the window; after that the `Window` owns the real size and this is just the
     * last value asked for. */
    math::Vec2<int32_t> m_windowSize { 800, 600 };

    bool m_shouldClose { false };
    bool m_isRunning { false };
  };
} // namespace ROSE