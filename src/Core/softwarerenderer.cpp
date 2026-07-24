/**

  @file       softwarerenderer.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       20.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <ROSE/ROSE.h>
#include <SDL3/SDL.h>
using namespace ROSE;

SoftwareRenderer::SoftwareRenderer() = default;
SoftwareRenderer::~SoftwareRenderer() {
  if (m_renderer) SDL_DestroyRenderer(static_cast<SDL_Renderer *>(m_renderer));
}

BackendStatus SoftwareRenderer::Init(const RenderBackendContext &ctx) {
  SDL_Window *window = static_cast<SDL_Window *>(ctx.window.ptr);
  SDL_SetWindowSize(window, ctx.width, ctx.height);
  if (!window)
    return BackendStatus::WindowUnavailable;
  Log(LogLevel::Debug, "Available rendering drivers:\n");
  int n = SDL_GetNumRenderDrivers();
  if (n < 1) {
    ROSE_LOG_DEBUG("\tnone\n");
    ROSE_LOG_ERROR("No compatible rendering drivers available! SDL Error: {}\n", SDL_GetError());
    return BackendStatus::UnsupportedHardware;
  }
  for (int i = 0; i < n; ++i) {
    Log(LogLevel::Debug, "\t{}\n", SDL_GetRenderDriver(i));
  }
  SDL_Renderer *renderer = SDL_CreateRenderer(window, "vulkan, direct3d11, opengl, gpu, software");
  m_renderer = renderer;
  BackendStatus status;
  if (renderer) {
    Log(LogLevel::Trace, "SDL Renderer sucessfully created. Driver chosen: {}\n", SDL_GetRendererName(renderer));
  } else {
    Log(LogLevel::Error, "Renderer creation failed! SDL Error: {}\n", SDL_GetError());
    return BackendStatus::Failure;
  }

  if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer)) {
    ROSE_LOG_ERROR("ImGui SDL3 Impl failed!\n");
    return BackendStatus::Failure;
  }
  if (!ImGui_ImplSDLRenderer3_Init(renderer)) {
    ROSE_LOG_ERROR("ImGui SDL Renderer Impl failed!\n");
    return BackendStatus::Failure;
  }
  return BackendStatus::Success;
}

void SoftwareRenderer::Shutdown() {
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();

}

void SoftwareRenderer::BeginFrame() {
  SDL_SetRenderDrawColor(static_cast<SDL_Renderer *>(m_renderer), 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(static_cast<SDL_Renderer *>(m_renderer));

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
}

void SoftwareRenderer::EndFrame() {
  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), static_cast<SDL_Renderer *>(m_renderer));
  SDL_RenderPresent(static_cast<SDL_Renderer *>(m_renderer));
}

void SoftwareRenderer::OnResize(int width, int height) {}

void *SoftwareRenderer::GetNativeHandle() const { return m_renderer; }
const char *SoftwareRenderer::GetName() const { return "Software renderer"; }
