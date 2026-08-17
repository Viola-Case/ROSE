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
#include <ROSE/Core/api.h>
#include <ROSE/Core/factory.h>
#include <ROSE/Core/gfx.h>
#include <ROSE/Core/window.h>

namespace ROSE {

  class Scene;
  class SceneManager;

  /*!
   * Not to be confused with `ApplicationFlag`
   */
  using ApplicationFlags = uint64_t;

  /*!
   * Not to be confused with `ApplicationFlags`
   */
  struct ROSE_API(Core) ApplicationFlag {
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
  constexpr ApplicationFlags APPLICATION_NO_SOUND = static_cast<ApplicationFlags>(1)
                                                    << static_cast<int>(ApplicationFlag::NoSound);
  constexpr ApplicationFlags APPLICATION_VULKAN = static_cast<ApplicationFlags>(1)
                                                  << static_cast<int>(ApplicationFlag::Vulkan);
  constexpr ApplicationFlags APPLICATION_OPENGL = static_cast<ApplicationFlags>(1)
                                                  << static_cast<int>(ApplicationFlag::OpenGL);
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

  //! What an application gets when it asks for nothing in particular: a windowed, gamepad-aware
  //! game on the SDL renderer.
  constexpr ApplicationFlags APPLICATION_DEFAULT = APPLICATION_CONTROLLER_SUPPORT | APPLICATION_SOFTWARE_RENDERER;


  /*!
   * Everything `Application::Init` needs, in one object. `Application` deliberately has no
   * pre-init setters at all: anything that has to be true before the window and the renderer
   * exist belongs here, and everything here is consumed exactly once, by `Init`.
   *
   * Setters chain, so a whole configuration is one expression:
   *
   * @code
   * ApplicationInitSettings settings { "Orbits" };
   * settings.SetWindowSize(800, 800).SetVSync(true).AddSceneFromFile("assets/orbits.json");
   * if (const int err = app.Init(Move(settings))) return err;
   * @endcode
   *
   * Move-only, because it owns the scenes and `Scene` is move-only. `Init` takes it by rvalue
   * and moves the scenes out of it; the settings object is spent afterwards.
   */
  class ROSE_API(Core) ApplicationInitSettings final {
    friend class Application;

  public:
    /* Every one of these is defaulted, but out of line: `Scene` is only forward-declared here, and
     * defining them inline would instantiate `List<Scene>`'s destructor against an incomplete type
     * in every translation unit that includes this header. */
    ApplicationInitSettings() noexcept;
    explicit ApplicationInitSettings(StringView _title);
    ~ApplicationInitSettings();

    ApplicationInitSettings(ApplicationInitSettings &&) noexcept;
    ApplicationInitSettings &operator=(ApplicationInitSettings &&) noexcept;
    ApplicationInitSettings(const ApplicationInitSettings &) = delete;
    ApplicationInitSettings &operator=(const ApplicationInitSettings &) = delete;

    //! Window caption, and the name the log and the per-user directories are filed under.
    ApplicationInitSettings &SetTitle(StringView) noexcept;
    //! Vendor/author name. Only used to build the per-user paths - see `PathSettings`.
    ApplicationInitSettings &SetOrganization(StringView) noexcept;

    //! Size the window is created at. Ignored when `Headless`.
    ApplicationInitSettings &SetWindowSize(math::Vec2<int>) noexcept;
    inline ApplicationInitSettings &SetWindowSize(int _width, int _height) noexcept {
      return SetWindowSize({ _width, _height });
    }

    ApplicationInitSettings &SetFlags(ApplicationFlags) noexcept; //!< replaces the whole set
    ApplicationInitSettings &SetFlag(ApplicationFlag, bool) noexcept;

    ApplicationInitSettings &SetVSync(bool) noexcept;

    /*!
     * Frame cap in frames per second. `0` means uncapped, which still yields a millisecond a
     * frame so the loop does not spin a core flat. Independent of `SetVSync` - with vsync on,
     * whichever limit is tighter wins.
     */
    ApplicationInitSettings &SetTargetFrameRate(uint32_t) noexcept;

    /*!
     * Adopt a window that already exists rather than creating one - the editor embedding a game
     * viewport, or a host application handing over a child window. `nullptr` (the default) means
     * create one.
     *
     * @todo Not honoured yet: `Window` can only `Create`, so `Init` logs and creates its own.
     *       Wants a `Window::Adopt(void *)` built on SDL_CreateWindowWithProperties.
     */
    ApplicationInitSettings &SetExistingWindow(void *_handle) noexcept;

    //! The first scene added is the one the application starts on.
    ApplicationInitSettings &AddScene(Scene &&) noexcept;
    ApplicationInitSettings &SetScenes(List<Scene> &&) noexcept; //!< replaces anything already added

    /*!
     * Reads the file whole and parses it with `Scene::FromJSONString`. On failure the settings
     * are left untouched and the error is logged - check `GetScenes().size()` if you care.
     */
    ApplicationInitSettings &AddSceneFromFile(StringView _path) noexcept;

    [[nodiscard]] StringView GetTitle() const noexcept { return m_title; }
    [[nodiscard]] StringView GetOrganization() const noexcept { return m_organization; }
    [[nodiscard]] math::Vec2<int> GetWindowSize() const noexcept { return m_windowSize; }
    [[nodiscard]] ApplicationFlags GetFlags() const noexcept { return m_flags; }
    [[nodiscard]] bool GetFlag(ApplicationFlag _flag) const noexcept {
      return m_flags & (static_cast<ApplicationFlags>(1) << _flag);
    }
    [[nodiscard]] bool GetVSync() const noexcept { return m_vsync; }
    [[nodiscard]] uint32_t GetTargetFrameRate() const noexcept { return m_targetFrameRate; }
    [[nodiscard]] void *GetExistingWindow() const noexcept { return m_windowHandle; }
    [[nodiscard]] const List<Scene> &GetScenes() const noexcept { return m_scenes; }

  private:
    String m_title { "Application" };
    String m_organization {};
    math::Vec2<int> m_windowSize { 800, 600 };
    ApplicationFlags m_flags { APPLICATION_DEFAULT };
    uint32_t m_targetFrameRate { 0 }; //!< 0 = uncapped
    void *m_windowHandle { nullptr }; //!< If not `nullptr`, adopt this window instead of creating one
    List<Scene> m_scenes {};
    void *m_customBackend { nullptr }; //!< If not `nullptr`, called as factory function of custom renderer
    bool m_vsync { false };
  };


  class ROSE_API(Core) Application final {

  public:
    Application() noexcept;
    ~Application();

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;
    Application(Application &&) = delete;
    Application &operator=(Application &&) = delete;

    /*!
     * Brings up the subsystems, the window, the renderer and the scenes described by `_settings`,
     * which is consumed. Call exactly once, before `Run`.
     *
     * @retval 0 on success, negative otherwise.
     */
    int Init(ApplicationInitSettings &&_settings);
    void Run(); //!< Should only be called once. Terminates if called again.
    void Quit() noexcept;
    // void CleanUp();

    [[nodiscard]] const char *GetTitle() const noexcept;
    [[nodiscard]] const char *GetOrganization() const noexcept;

    /*!
     * @retval the owned window, or `nullptr` when the application runs headless. Always check
     *         before dereferencing - a headless run legitimately has no window.
     */
    [[nodiscard]] Window *GetWindow() noexcept;
    [[nodiscard]] const Window *GetWindow() const noexcept;

    /*!
     * @retval the scene currently being updated, or `nullptr` before `Init` and when the
     *         application was initialized with no scenes at all.
     */
    [[nodiscard]] Scene *GetCurrentScene() noexcept;

    const List<Scene> &GetScenes() noexcept;

    [[nodiscard]] bool GetFlag(ApplicationFlag) const noexcept;

    [[nodiscard]] uint32_t GetTargetFrameRate() const noexcept;
    void SetTargetFrameRate(uint32_t _fps) noexcept; //!< 0 = uncapped; takes effect next frame

    void SetWindowSize(math::Vec2<int> size) noexcept;
    inline void SetWindowSize(int width, int height) noexcept { SetWindowSize({ width, height }); }

    /* TODO Unimplemented, and pre-init by nature - a module has to be loaded before any scene
     * that names its behaviors is deserialized. Once dynamic loading exists this should become a
     * module list on ApplicationInitSettings that Init walks, not a member function to call
     * beforehand. */
    bool LoadModule(StringView name) noexcept;

  protected:
    String m_title { "game" };
    String m_organization {};

    List<Scene> m_scenes {};
    Scene *m_currentScene { nullptr };

    // AppID m_id { 0 };

    UniquePtr<Window> m_window {};

    ApplicationFlags m_flags { 0 };

    RenderBackend *m_renderer { nullptr };

    UniquePtr<SceneManager> m_manager {};

    /* The size the window is created at, taken from the init settings. Authoritative only until
     * `Init` builds the window; after that the `Window` owns the real size and this is just the
     * last value asked for. */
    math::Vec2<int32_t> m_windowSize { 800, 600 };

    uint32_t m_targetFrameRate { 0 };

    bool m_vsync { true };
    bool m_shouldClose { false };
    bool m_isRunning { false };
  };
} // namespace ROSE
