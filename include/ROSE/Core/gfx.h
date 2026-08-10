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
#include <ROSE/Core/paramview.h>

namespace ROSE {

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

  /*!
   * This hosts all the abstraction of the graphics backend.
   */
  class RenderBackend {

  public:
    RenderBackend() = default;
    virtual ~RenderBackend() = default;
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
  };

  class OpenGLRenderer : public RenderBackend {
  public:
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
    void *m_context { nullptr };
    void *m_window { nullptr }; //!< `SDL_Window *`, kept for the buffer swap in `EndFrame`
    String m_name { "OpenGL" }; //!< built by `Init` from the context the driver actually handed back
    int m_versionMajor;
    int m_versionMinor;
    Vec4f m_backgroundColor {0.f, 0.f, 0.f, 1.f};
  };

  class SoftwareRenderer : public RenderBackend {
  public:
    SoftwareRenderer();
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

  private:
    void *m_renderer;
  };

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
    const char *GetName() const override { return ""; }
  };





} // namespace ROSE