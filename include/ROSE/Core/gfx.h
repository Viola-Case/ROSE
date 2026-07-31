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

  struct WindowHandle {
    void *ptr;
  };

  struct RenderBackendContext {
    WindowHandle window;

    int width;
    int height;
    bool vsync;
    ParamView *config;
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
    ~OpenGLRenderer();

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

    void UpdateBuffers();
  protected:
    void *m_context;
    int m_versionMajor;
    int m_versionMinor;
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





} // namespace ROSE