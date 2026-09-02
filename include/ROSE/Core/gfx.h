/**

  @file      gfx.h
  @brief
  @details   ~
  @author    Viola Case
  @date      07.04.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/

#pragma once

#include <cstdint>
#include <ROSE/Core/api.h>
#include <ROSE/Core/math.h>
#include <ROSE/Core/paramview.h>
#include <ROSE/Core/rtl.h>
#include <ROSE/Core/uuid.h>

namespace ROSE {

  class Renderable;

  /*!
   * Only ever populated from `Window::GetHandle()`.
   */
  struct WindowHandle {
    void *ptr { nullptr };
  };

  struct RenderBackendContext {
    WindowHandle window {};

    int width { 0 };
    int height { 0 };
    bool vsync { false };
    ParamView *config { nullptr };
  };

  enum class BackendStatus : uint32_t {
    Success = 0,
    WindowUnavailable,
    ContextCreationFailed,
    UnsupportedHardware,
    Failure,
    IHaveNoIdea
  };

#pragma region draw vocabulary

  /*!
   * Not to be confused with `RenderableFlag`. Bit set, same idiom as `ApplicationFlags`.
   */
  using RenderableFlags = uint32_t;

  /*!
   * Not to be confused with `RenderableFlags`. Bit indices, not masks - the masks are the
   * `RENDERABLE_*` constants below.
   */
  struct ROSE_API(CORE) RenderableFlag {
    enum Value : uint8_t {
      Transparent = 0, //!< blend, and draw after the opaque set
      ScreenSpace = 1, //!< positions are already window pixels; the camera transform is skipped
      Overlay     = 2, //!< drawn last, ignores depth (UI)
      Textured    = 3, //!< honour `DrawCommand::texture`
    } value;
    constexpr RenderableFlag(Value v) : value(v) {}
    constexpr operator uint32_t() const noexcept { return value; }
  };

  constexpr RenderableFlags RENDERABLE_TRANSPARENT = static_cast<RenderableFlags>(1)
                                                     << static_cast<int>(RenderableFlag::Transparent);
  constexpr RenderableFlags RENDERABLE_SCREEN_SPACE = static_cast<RenderableFlags>(1)
                                                      << static_cast<int>(RenderableFlag::ScreenSpace);
  constexpr RenderableFlags RENDERABLE_OVERLAY = static_cast<RenderableFlags>(1)
                                                 << static_cast<int>(RenderableFlag::Overlay);
  constexpr RenderableFlags RENDERABLE_TEXTURED = static_cast<RenderableFlags>(1)
                                                  << static_cast<int>(RenderableFlag::Textured);

  /*!
   * How a command's vertices group into primitives. All three consume vertices (or indices, when
   * present) in fixed-size groups - `Lines` means disjoint segments, two vertices each, never a
   * connected strip. A strip is expressible as segments; the reverse is not, and every backend
   * agrees on this reading.
   */
  enum class Topology : uint8_t {
    Points,    //!< one vertex each
    Lines,     //!< two vertices each, disjoint
    Triangles, //!< three vertices each
  };

  /*!
   * A vertex as it crosses the behavior/backend boundary. Float, not double: `Mesh` stores
   * `Vec3d` but every backend wants floats, so the narrowing happens once, in `Collect`.
   */
  struct DrawVertex {
    Vec3f position; //!< world space, or window pixels when `ScreenSpace` is set (z ignored)
    Vec4f color;    //!< straight (non-premultiplied) RGBA, 0..1
    Vec2f texCoord;
  };

  /*!
   * A `TextureRegistry` key. `UUID::Invalid()` - which is also the default-constructed value -
   * means untextured.
   */
  using TextureID = UUID;

  /*!
   * One draw's worth of geometry.
   *
   * Non-owning. Every pointer here must stay valid for the whole render pass, not merely until
   * `Draw` returns: a backend is free to defer, and the software rasterizer's tile binner will.
   * `RenderFrame` guarantees it by holding the frame's commands until the last one is drawn, and
   * the vertex data itself belongs to the emitting `Renderable`, which must not touch it again
   * before the frame ends.
   */
  struct DrawCommand {
    const DrawVertex *vertices { nullptr };
    size_t vertexCount { 0 };
    const uint32_t *indices { nullptr }; //!< null for a non-indexed draw
    size_t indexCount { 0 };
    Topology topology { Topology::Triangles };
    TextureID texture {};                  //!< resolved by the backend; ignored without `Textured`
    RenderableFlags flags {};
    Mat4f transform { Mat4f::Identity() }; //!< object -> world; ignored when `ScreenSpace`
    /*! Points only. A wide line is geometry (a ribbon), not a backend feature - SDL has no line
     *  width and GL clamps it, so honouring it there would silently disagree per backend. */
    float pointSize { 1.0f };
  };

  /*!
   * What `Renderable::Collect` fills. A renderable may add zero, one, or many commands.
   *
   * A thin appender onto the frame's shared command buffer rather than a container of its own,
   * so a frame costs one allocation amortised rather than one per renderable. Only
   * `RenderBackend` can make one.
   */
  class ROSE_API(CORE) RenderList {
    friend class RenderBackend;

  public:
    RenderList(const RenderList &) = delete;
    RenderList &operator=(const RenderList &) = delete;

    void Add(const DrawCommand &) noexcept;

    [[nodiscard]] size_t Size() const noexcept; //!< commands added through *this*
    [[nodiscard]] bool Empty() const noexcept;

  private:
    explicit RenderList(List<DrawCommand> &_sink) noexcept : m_sink(&_sink) {}

    List<DrawCommand> *m_sink;
    size_t m_added { 0 };
  };

#pragma endregion

  /*!
   * This hosts all the abstraction of the graphics backend.
   *
   * A backend implements the lifecycle below plus exactly one drawing operation, `Draw`.
   * Everything about a frame that is not backend-specific - which renderables contribute, what
   * order they draw in, whether they are enabled - lives here and is shared, so adding a backend
   * touches no behavior.
   */
  class ROSE_API(CORE) RenderBackend {

  public:
    RenderBackend() = default;
    virtual ~RenderBackend();
    RenderBackend(const RenderBackend &) = delete;
    RenderBackend(RenderBackend &&) = delete;
    RenderBackend &operator=(const RenderBackend &) = delete;
    RenderBackend &operator=(RenderBackend &&) = delete;

    virtual BackendStatus Init(const RenderBackendContext &) = 0;
    virtual void Shutdown() = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0; // includes present, until you have a reason to split them

    virtual void OnResize(int width, int height) = 0;

    virtual void *GetNativeHandle() const = 0;
    virtual const char *GetName() const = 0; // for logs and humans

#pragma region render pass

    /*!
     * Enrollment. Called by `Renderable::OnCreate`; not for general use.
     *
     * Idempotent: a renderable already enrolled here is left where it is. `Application::Run`
     * replays `OnCreate` over a scene it switches back to, and that must not enroll twice.
     */
    void Enroll(Renderable &) noexcept;
    void Withdraw(Renderable &) noexcept; //!< O(1); swap-and-pop against the cached slot

    /*!
     * The render pass. Collects from every enabled enrolled renderable, orders the result, and
     * draws it. Called once per frame by `Application::Run`, after `Scene::FrameUpdate` so that
     * a renderable's `Collect` sees the state its `FrameUpdate` just produced.
     *
     * Order is imposed here rather than inherited from iteration, which is unspecified:
     * opaque, then transparent, then overlay; within a band by `Renderable::GetLayer()`; ties
     * broken by enrollment order, so a frame is reproducible.
     */
    void RenderFrame();

    void SetViewProjection(const Mat4f &) noexcept;
    [[nodiscard]] const Mat4f &GetViewProjection() const noexcept { return m_viewProjection; }

#pragma endregion

  protected:
    //! The one backend-specific drawing operation. Everything else about a frame is shared.
    virtual void Draw(const DrawCommand &) = 0;

    /*!
     * Nulls every enrolled renderable's back-pointer and empties the registry.
     *
     * Ownership runs the wrong way at teardown: `~Application` deletes the backend in its body
     * but `m_scenes` - and so every `Renderable` - dies afterwards, as members. Calling this
     * from `~RenderBackend` means those renderables find a null back-pointer instead of a freed
     * backend. A `Shutdown()` may call it early; the destructor is the guarantee, because a
     * backend can be deleted without `Shutdown` ever running.
     */
    void DetachAllRenderables() noexcept;

    List<Renderable *> m_renderables {};
    Mat4f m_viewProjection { Mat4f::Identity() };

  private:
    /* Frame scratch, kept between frames for its capacity. m_frameCommands must outlive every
     * Draw call in the pass - see the lifetime note on DrawCommand. */
    List<DrawCommand> m_frameCommands {};

    struct SortEntry {
      uint32_t band;
      int32_t layer;
      uint32_t sequence;
      uint32_t index;
    };
    List<SortEntry> m_frameOrder {};
  };

  /*!
   * SDL's own 2D renderer, running on whatever GPU API SDL picks for the platform. This is what
   * the engine has always used; it was called `SoftwareRenderer` until the class below became a
   * real one.
   */
  class ROSE_API(CORE) SDLRenderer : public RenderBackend {
  public:
    SDLRenderer();
    ~SDLRenderer() override;
    SDLRenderer(const SDLRenderer &) = delete;
    SDLRenderer(SDLRenderer &&) = delete;
    SDLRenderer &operator=(const SDLRenderer &) = delete;
    SDLRenderer &operator=(SDLRenderer &&) = delete;

    BackendStatus Init(const RenderBackendContext &) override;
    void Shutdown() override;

    void BeginFrame() override;
    void EndFrame() override;

    void OnResize(int width, int height) override;

    void *GetNativeHandle() const override;
    const char *GetName() const override;

  protected:
    void Draw(const DrawCommand &) override;

  private:
    void *ResolveTexture(const TextureID &) noexcept; //!< `SDL_Texture *`, cached

    void *m_presenter { nullptr };            //!< `SDLPresenter`
    TypedHashMap<UUID, void *> m_textures {}; //!< `SDL_Texture *` per registry id

    /* Per-frame scratch, kept for its capacity. SDL wants tightly packed arrays of its own
     * shapes, so points and rects cannot simply alias m_scratch's stride. Vec2f and Vec4f are
     * layout-compatible with SDL_FPoint and SDL_FRect respectively; the .cpp asserts it. */
    List<DrawVertex> m_scratch {}; //!< transformed vertices
    List<Vec2f> m_scratchXY {};    //!< `SDL_FPoint`
    List<Vec4f> m_scratchRects {}; //!< `SDL_FRect`
  };

  /*!
   * Core-profile OpenGL backend over an SDL window created with `SDL_WINDOW_OPENGL`.
   *
   * One fixed shader program, one interleaved streaming VBO/IBO, a UUID-keyed texture cache, and
   * no depth testing: draw order is entirely the band/layer sort in `RenderFrame`. ImGui is drawn
   * last, in `EndFrame`, directly to the default framebuffer.
   *
   * `Init` only sets the *context* GL attributes (version, profile). Pixel-format attributes such
   * as `SDL_GL_DEPTH_SIZE` and multisampling are read when the window is created and must be set
   * before `Window::Create` in `Application::Init`.
   *
   * See `docs/opengl-pipeline.md` for the frame walkthrough and the extension points.
   */
  class ROSE_API(CORE) OpenGLRenderer : public RenderBackend {
  public:
    //! The requested core-profile version. The driver may hand back something newer; `GetName` reports it.
    explicit OpenGLRenderer(int majorVersion = 4, int minorVersion = 5);
    ~OpenGLRenderer() override;

    OpenGLRenderer(const OpenGLRenderer &) = delete;
    OpenGLRenderer(OpenGLRenderer &&) = delete;
    OpenGLRenderer &operator=(const OpenGLRenderer &) = delete;
    OpenGLRenderer &operator=(OpenGLRenderer &&) = delete;

    BackendStatus Init(const RenderBackendContext &) override;
    void Shutdown() override;
    void BeginFrame() override;
    void EndFrame() override;

    void OnResize(int width, int height) override;
    void *GetNativeHandle() const override;
    const char *GetName() const override;

  protected:
    void Draw(const DrawCommand &) override;

    void *m_context { nullptr }; //!< `SDL_GLContext`; null means not initialised, and `Shutdown` is a no-op
    void *m_window { nullptr }; //!< `SDL_Window *`, kept for the buffer swap in `EndFrame`
    String m_name { "OpenGL" }; //!< built by `Init` from the context the driver actually handed back
    int m_versionMajor;
    int m_versionMinor;
    Vec4f m_backgroundColor { 0.f, 0.f, 0.f, 1.f }; //!< TODO not yet read by `BeginFrame`, which clears to black

  private:
    uint32_t ResolveTexture(const TextureID &) noexcept; //!< GL texture name, cached

    TypedHashMap<UUID, uint32_t> m_textures {}; //!< GL name per registry id; 0 is cached for misses, never evicted
    bool BuildPipeline() noexcept; //!< the one built-in program, plus its streaming buffers

    uint32_t m_program { 0 }; //!< the single linked program; 0 makes `Draw` a no-op
    uint32_t m_vao { 0 }, m_vbo { 0 }, m_ibo { 0 }; //!< one VAO with the `DrawVertex` layout, one streaming VBO and IBO
    //! Uniform locations, fetched once after link. -1 (not found) is silently ignored by GL.
    int m_uViewProjection { -1 }, m_uModel { -1 }, m_uUseTexture { -1 };
    int m_uScreenSpace { -1 }, m_uViewport { -1 };
    int m_viewportWidth { 0 }, m_viewportHeight { 0 }; //!< fed to `uViewport` for screen-space commands; logical size, not drawable (HiDPI TODO)
  };

  /*!
   * A real software rasterizer. Owns its framebuffer, draws into plain memory, and blits once
   * per frame through a streaming `SDL_Texture`.
   *
   * The internal resolution is deliberately decoupled from the window: rendering a fixed 640x360
   * and letting the present stretch it is the single largest performance lever available, and
   * ImGui draws *after* the blit so the HUD stays crisp at native size over a chunky world.
   */
  class ROSE_API(CORE) SoftwareRenderer : public RenderBackend {
  public:
    //! 0, 0 means "track the window". A fixed internal size is the point of the thing.
    explicit SoftwareRenderer(int internalWidth = 0, int internalHeight = 0);
    ~SoftwareRenderer() override;
    SoftwareRenderer(const SoftwareRenderer &) = delete;
    SoftwareRenderer(SoftwareRenderer &&) = delete;
    SoftwareRenderer &operator=(const SoftwareRenderer &) = delete;
    SoftwareRenderer &operator=(SoftwareRenderer &&) = delete;

    BackendStatus Init(const RenderBackendContext &) override;
    void Shutdown() override;

    void BeginFrame() override;
    void EndFrame() override;

    void OnResize(int width, int height) override;

    void *GetNativeHandle() const override;
    const char *GetName() const override;

    //! Resize the framebuffer. 0, 0 goes back to tracking the window.
    void SetInternalResolution(int width, int height);
    [[nodiscard]] int GetInternalWidth() const noexcept { return m_width; }
    [[nodiscard]] int GetInternalHeight() const noexcept { return m_height; }

  protected:
    void Draw(const DrawCommand &) override;

  private:
    void Resize(int width, int height); //!< reallocates the framebuffer and the blit target
    void ClearTo(uint32_t argb) noexcept;
    void BlendPixel(int x, int y, const Vec4f &rgba, bool blend) noexcept;
    void FillTriangle(const DrawVertex &a, const DrawVertex &b, const DrawVertex &c, bool blend) noexcept;
    void RasterLine(const DrawVertex &a, const DrawVertex &b, bool blend) noexcept;
    void RasterPoint(const DrawVertex &v, float size, bool blend) noexcept;

    List<uint32_t> m_color {};     //!< ARGB8888, row-major, `m_width * m_height`
    List<DrawVertex> m_scratch {}; //!< transformed vertices; keeps capacity per frame
    int m_width { 0 }, m_height { 0 };
    int m_requestedWidth { 0 }, m_requestedHeight { 0 }; //!< 0 = track the window
    void *m_presenter { nullptr }; //!< `SDLPresenter`
    void *m_target { nullptr };    //!< streaming `SDL_Texture`, rebuilt only on resolution change
    uint32_t m_clear { 0xFF000000 };
  };

  /*!
   * Draws nothing, successfully. The natural backend for a headless run, and the thing to
   * inherit from when a new backend is only half-written.
   */
  class NoopRenderer : public RenderBackend {
  public:
    NoopRenderer() = default;
    ~NoopRenderer() override = default;
    NoopRenderer(const NoopRenderer &) = delete;
    NoopRenderer(NoopRenderer &&) = delete;
    NoopRenderer &operator=(const NoopRenderer &) = delete;
    NoopRenderer &operator=(NoopRenderer &&) = delete;
    BackendStatus Init(const RenderBackendContext &) override { return BackendStatus::IHaveNoIdea; }
    void Shutdown() override {}
    void BeginFrame() override {}
    void EndFrame() override {}
    void OnResize(int width, int height) override {}
    void *GetNativeHandle() const override { return nullptr; }
    const char *GetName() const override { return "Noop"; }

  protected:
    void Draw(const DrawCommand &) override {}
  };

} // namespace ROSE
