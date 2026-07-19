/**

  @file      gfx.h
  @brief
  @details   ~
  @author    Viola Case
  @date      07.04.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/

#pragma once

namespace ROSE {
  class Application;

  enum class RenderBackend : unsigned char {
    Software,
    OpenGL,
    Vulkan,
    // DirectX,
    // Metal
  };

  /*!
   * This will eventually host all the abstraction of the graphics backend (including the software renderer)
   *
   * Should probably make the name something nicer
   *
   * @todo decide if this will also handle the window (i am thinking maybe)
   */
  class GraphicsBackend {
    void *m_handle {nullptr};
    RenderBackend m_renderBackend;
  public:
    GraphicsBackend(RenderBackend backend, const Application &);
    ~GraphicsBackend();
    GraphicsBackend(const GraphicsBackend &) = delete;
    GraphicsBackend(GraphicsBackend &&) = delete;
    GraphicsBackend &operator=(const GraphicsBackend &) = delete;
    GraphicsBackend &operator=(GraphicsBackend &&) = delete;
    void *GetHandle() const noexcept;
    void RegisterCustomBackend(void *);
  };





} // namespace ROSE