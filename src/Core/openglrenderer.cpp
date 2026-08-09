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

/*!
 * The GLSL `#version` directive matching a GL version, for ImGui's GL3 backend.
 *
 * GLSL version numbers only track GL version numbers from 3.3 onwards - before that they run
 * 110/120/130/140/150 - so the obvious "major, minor, 0" spelling is wrong below 3.3. Only 3.2
 * is reachable here in practice: `Init` always requests a core profile, and core profiles do not
 * exist before 3.2, which is exactly the one version the naive spelling gets wrong.
 *
 * `constexpr` rather than `consteval` on purpose - the version is a runtime constructor argument,
 * so it has to be callable with a non-constant. The `static_assert`s below still evaluate it at
 * compile time.
 *
 * @retval nullptr for a version with no core-profile GLSL mapping. That is also the value
 *         `ImGui_ImplOpenGL3_Init` reads as "use your own default", so it stays a usable argument.
 */
constexpr const char *GLSLVersionDirective(const int _major, const int _minor) noexcept {
  if (_major == 3) {
    if (_minor == 2) return "#version 150 core";
    if (_minor == 3) return "#version 330 core";
    return nullptr;
  }
  if (_major != 4) return nullptr;
  switch (_minor) {
  case 0:
    return "#version 400 core";
  case 1:
    return "#version 410 core";
  case 2:
    return "#version 420 core";
  case 3:
    return "#version 430 core";
  case 4:
    return "#version 440 core";
  case 5:
    return "#version 450 core";
  case 6:
    return "#version 460 core";
  default:
    return nullptr;
  }
}

/* Only used by the assertions below. Compares element-wise rather than going through the RTL's
 * MemCmp, whose `a == b` fast path is not constant-evaluable on two string literals - the compiler
 * is free to merge identical literals or not, so the address comparison has no defined answer. */
constexpr bool SameString(const char *_a, const char *_b) noexcept {
  if (!_a || !_b) return false;
  for (size_t i = 0;; ++i) {
    if (_a[i] != _b[i]) return false;
    if (_a[i] == '\0') return true;
  }
}

static_assert(SameString(GLSLVersionDirective(4, 5), "#version 450 core"),
              "the default 4.5 context must map to GLSL 450");
static_assert(SameString(GLSLVersionDirective(3, 2), "#version 150 core"),
              "GL 3.2 maps to GLSL 150, not 320 - the whole reason this table is not arithmetic");
static_assert(GLSLVersionDirective(3, 1) == nullptr, "core profiles do not exist before GL 3.2");
static_assert(GLSLVersionDirective(4, 7) == nullptr, "an unknown version has to fall through to ImGui's default");

OpenGLRenderer::OpenGLRenderer(int majorVersion, int minorVersion)
    : m_versionMajor(majorVersion), m_versionMinor(minorVersion) {};

OpenGLRenderer::~OpenGLRenderer() { Shutdown(); }

BackendStatus OpenGLRenderer::Init(const RenderBackendContext &ctx) {
  SDL_Window *window = static_cast<SDL_Window *>(ctx.window.ptr);
  if (!window) {
    ROSE_LOG_ERROR("No window to attach a GL context to!\n");
    return BackendStatus::WindowUnavailable;
  }

  /* Only the three context attributes below are read by SDL at context creation, so setting them
   * here works. The pixel-format ones - SDL_GL_DEPTH_SIZE, SDL_GL_DOUBLEBUFFER, SDL_GL_MULTISAMPLE*
   * - are read at *window* creation and are inert from here; they belong next to Window::Create. */
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, m_versionMajor);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, m_versionMinor);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (!context) {
    /* A window created without SDL_WINDOW_OPENGL fails here, and that is the usual cause. */
    ROSE_LOG_ERROR("GL context creation failed - is the window SDL_WINDOW_OPENGL? SDL Error: {}\n", SDL_GetError());
    return BackendStatus::ContextCreationFailed;
  }

  m_context = context;
  m_window = window;

  /* Has to come before anything that touches GL, ImGui's backend included - its Init calls
   * glGetString, which is a null function pointer until glad has filled the table in. */
  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    ROSE_LOG_ERROR("Failed to load GL {}.{} function pointers!\n", m_versionMajor, m_versionMinor);
    SDL_GL_DestroyContext(context);
    m_context = nullptr;
    m_window = nullptr;
    return BackendStatus::UnsupportedHardware;
  }

  /* Ask what we actually got rather than reporting what we asked for - requesting 4.5 commonly
   * hands back 4.6, and the log should say which. */
  int major = m_versionMajor;
  int minor = m_versionMinor;
  SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
  SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
  m_name = Format("OpenGL {}.{}", major, minor);

  if (!ImGui_ImplSDL3_InitForOpenGL(window, context)) {
    ROSE_LOG_ERROR("ImGui SDL3 Impl failed!\n");
    SDL_GL_DestroyContext(context);
    m_context = nullptr;
    m_window = nullptr;
    return BackendStatus::Failure;
  }

  /* Deliberately the *requested* version, not the one queried above: a driver only ever hands
   * back a context at least as new as what was asked for, so this is always a version the
   * context can compile. */
  const char *glslVersion = GLSLVersionDirective(m_versionMajor, m_versionMinor);
  if (!glslVersion) {
    ROSE_LOG_WARN("No GLSL version known for a GL {}.{} core context, letting ImGui pick.\n", m_versionMajor,
                  m_versionMinor);
  }

  if (!ImGui_ImplOpenGL3_Init(glslVersion)) {
    ROSE_LOG_ERROR("ImGui OpenGL3 Impl failed!\n");
    ImGui_ImplSDL3_Shutdown();
    SDL_GL_DestroyContext(context);
    m_context = nullptr;
    m_window = nullptr;
    return BackendStatus::Failure;
  }

  SDL_GL_SetSwapInterval(ctx.vsync ? 1 : 0);

  glViewport(0, 0, ctx.width, ctx.height);

  Log(LogLevel::Trace, "GL context sucessfully created. Version: {}\n", m_name.c_str());

  return BackendStatus::Success;
}

void OpenGLRenderer::Shutdown() {
  /* Runs twice on a normal exit - once from Application::Run, once from the destructor - so
   * everything here is guarded on the context and clears it on the way out. */
  if (!m_context) return;

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();

  SDL_GL_DestroyContext(static_cast<SDL_GLContext>(m_context));
  m_context = nullptr;
  m_window = nullptr;
}

void OpenGLRenderer::BeginFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL3_NewFrame();

  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::EndFrame() {
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow(static_cast<SDL_Window *>(m_window));
}

/* TODO On a HiDPI display the drawable is larger than the window, so this wants
 * SDL_GetWindowSizeInPixels rather than the logical size the resize event carries. */
void OpenGLRenderer::OnResize(int width, int height) {
  if (!m_context) return;
  glViewport(0, 0, width, height);
}

const char *OpenGLRenderer::GetName() const { return m_name.c_str(); }

void *OpenGLRenderer::GetNativeHandle() const { return m_context; }
