/**

  @file       openglrenderer.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       20.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#include <ROSE/ROSE.h>
#include <SDL3/SDL.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <glad/glad.h>

using namespace ROSE;

OpenGLRenderer::OpenGLRenderer(int majorVersion, int minorVersion)
    : m_versionMajor(majorVersion), m_versionMinor(minorVersion) {};
OpenGLRenderer::~OpenGLRenderer() { SDL_GL_DestroyContext(static_cast<SDL_GLContext>(m_context)); }
BackendStatus OpenGLRenderer::Init(const RenderBackendContext &ctx) {

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, m_versionMajor);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, m_versionMinor);

  SDL_Window *window = static_cast<SDL_Window *>(ctx.window.ptr);
  if (!window) {
    return BackendStatus::WindowUnavailable;
  }
  SDL_GLContext context = SDL_GL_CreateContext(window);

  if (!context) {
    return BackendStatus::Failure;
  }
  m_context = context;
  ImGui_ImplSDL3_InitForOpenGL(window, context);
  ImGui_ImplOpenGL3_Init(Format("#version {}{}0 core", m_versionMajor, m_versionMinor).c_str());



  int version = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
  if (!version) {
    glad_glGetError();
    SDL_GL_DestroyContext(context);
    return BackendStatus::Failure;
  }

  glViewport(0, 0, ctx.width, ctx.height);
}

void OpenGLRenderer::Shutdown() {

}

void OpenGLRenderer::BeginFrame() {

}

void OpenGLRenderer::EndFrame() {

}

const char *OpenGLRenderer::GetName() const {
  static char name[16];
  if (!name[0]) {
    String v;
    v.reserve(16);
    v = Format("OpenGL {}.{}", m_versionMajor, m_versionMinor);
    MemCpy(name, v.c_str(), 16);
  }
  return name;
}

void *OpenGLRenderer::GetNativeHandle() const {
  return m_context;
}
