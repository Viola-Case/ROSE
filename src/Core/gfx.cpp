/**

  @file       gfx.cpp
  @brief      
  @details    ~
  @author     Viola Case
  @date       19.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/

#include <ROSE/Core/gfx.h>

#include <imgui_impl_sdl3.h>
#include <ROSE/Core/application.h>
#include <SDL3/SDL.h>

using namespace ROSE;

GraphicsBackend::GraphicsBackend(RenderBackend backend, const Application &app) : m_renderBackend(backend) {
  auto window = static_cast<SDL_Window *>(app.GetWindow());
  switch (backend) {
  case RenderBackend::Software:
    m_handle = SDL_CreateRenderer(window, nullptr);
    ImGui_ImplSDL3_InitForSDLRenderer(window, static_cast<SDL_Renderer *>(m_handle));
    break;
  default:
    break;
  }
}

GraphicsBackend::~GraphicsBackend() {
  SDL_DestroyRenderer(static_cast<SDL_Renderer *>(m_handle));
}

void *GraphicsBackend::GetHandle() const noexcept {
  return m_handle;
}


