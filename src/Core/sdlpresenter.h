/**

  @file       sdlpresenter.h
  @brief      Internal: the SDL_Renderer and ImGui plumbing shared by the SDL-presenting backends.
  @details    Not a public header - it is deliberately full of SDL types, which is exactly what
              `gfx.h` must never expose. `SDLRenderer` draws through the SDL_Renderer this owns;
              `SoftwareRenderer` rasterizes into its own memory and only uses it to blit and to
              keep ImGui's SDLRenderer3 backend alive.
  @author     Viola Case
  @date       29.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <ROSE/ROSE.h>
#include <SDL3/SDL.h>

namespace ROSE {

  class SDLPresenter {
  public:
    BackendStatus Init(const RenderBackendContext &_ctx) {
      m_window = static_cast<SDL_Window *>(_ctx.window.ptr);
      if (!m_window) return BackendStatus::WindowUnavailable;

      SDL_SetWindowSize(m_window, _ctx.width, _ctx.height);

      Log(LogLevel::Debug, "Available rendering drivers:\n");
      const int n = SDL_GetNumRenderDrivers();
      if (n < 1) {
        ROSE_LOG_DEBUG("\tnone\n");
        ROSE_LOG_ERROR("No compatible rendering drivers available! SDL Error: {}\n", SDL_GetError());
        return BackendStatus::UnsupportedHardware;
      }
      for (int i = 0; i < n; ++i)
        Log(LogLevel::Debug, "\t{}\n", SDL_GetRenderDriver(i));

      /* No spaces: SDL splits this on ',' and compares each entry against the driver names
       * verbatim, so " opengl" never matches "opengl". With the spaces in, only the leading
       * "vulkan" was reachable and every fallback was dead -- which is why this worked on Windows
       * and died on a Wayland session whose Vulkan ICD has no VK_KHR_wayland_surface. */
      m_renderer = SDL_CreateRenderer(m_window, "vulkan,direct3d11,opengl,gpu,software");
      if (!m_renderer) {
        Log(LogLevel::Error, "Renderer creation failed! SDL Error: {}\n", SDL_GetError());
        return BackendStatus::Failure;
      }
      Log(LogLevel::Trace, "SDL Renderer sucessfully created. Driver chosen: {}\n",
          SDL_GetRendererName(m_renderer));

      if (!SDL_SetRenderVSync(m_renderer, _ctx.vsync ? 1 : SDL_RENDERER_VSYNC_DISABLED))
        ROSE_LOG_WARN("Could not set vsync on the renderer. SDL Error: {}\n", SDL_GetError());

      if (!ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer)) {
        ROSE_LOG_ERROR("ImGui SDL3 Impl failed!\n");
        return BackendStatus::Failure;
      }
      if (!ImGui_ImplSDLRenderer3_Init(m_renderer)) {
        ROSE_LOG_ERROR("ImGui SDL Renderer Impl failed!\n");
        return BackendStatus::Failure;
      }

      m_imguiUp = true;
      return BackendStatus::Success;
    }

    /*! Idempotent - Shutdown() may run before the destructor, and usually does. */
    void Shutdown() {
      if (m_imguiUp) {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        m_imguiUp = false;
      }
      if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
      }
    }

    ~SDLPresenter() { Shutdown(); }

    void NewFrame() const {
      ImGui_ImplSDLRenderer3_NewFrame();
      ImGui_ImplSDL3_NewFrame();
    }

    void Clear(const Vec4f &_rgba) const {
      SDL_SetRenderDrawColorFloat(m_renderer, _rgba.x, _rgba.y, _rgba.z, _rgba.w);
      SDL_RenderClear(m_renderer);
    }

    /*! ImGui draws last, at native resolution, over whatever the backend already put down. */
    void PresentWithImGui() const {
      ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
      SDL_RenderPresent(m_renderer);
    }

    [[nodiscard]] SDL_Renderer *Renderer() const noexcept { return m_renderer; }
    [[nodiscard]] SDL_Window *Window() const noexcept { return m_window; }

    [[nodiscard]] math::Vec2<int> OutputSize() const noexcept {
      int w = 0, h = 0;
      SDL_GetCurrentRenderOutputSize(m_renderer, &w, &h);
      return { w, h };
    }

  private:
    SDL_Renderer *m_renderer { nullptr };
    SDL_Window *m_window { nullptr };
    bool m_imguiUp { false };
  };

} // namespace ROSE
