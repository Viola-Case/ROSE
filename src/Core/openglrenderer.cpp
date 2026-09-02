/**

  @file       openglrenderer.cpp
  @brief      The OpenGL half of the render path: context, one fixed program, streaming buffers, Draw.
  @details    Everything backend-independent (enrollment, collect, sort) is in gfx.cpp and shared
              with the other backends; this file only turns an already-ordered DrawCommand into GL
              calls. The full walkthrough, including extension points, is docs/opengl-pipeline.md.

              Per-frame call order, driven by Application::Run:
                BeginFrame  -> ImGui new-frame hooks, clear colour + depth
                Draw x N    -> one upload + one draw call per DrawCommand, via RenderBackend::RenderFrame
                EndFrame    -> ImGui draw data on top, then buffer swap
  @author     Viola Case
  @date       20.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#include <ROSE/ROSE.h>
#include <SDL3/SDL.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <glad/gl.h>

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

/*!
 * Stage order matters and is fixed: context -> glad -> ImGui -> pipeline -> viewport. glad must
 * precede anything that calls a gl* symbol (ImGui's init included); ImGui must precede
 * BuildPipeline only so a failure there tears down in one place via Shutdown.
 *
 * Every failure after context creation undoes what it made and returns a status;
 * Application::Init treats anything but Success as fatal.
 */
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
  if (!gladLoadGL(SDL_GL_GetProcAddress)) {
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

  if (!BuildPipeline()) {
    ROSE_LOG_ERROR("Could not build the built-in GL pipeline!\n");
    Shutdown();
    return BackendStatus::Failure;
  }

  glViewport(0, 0, ctx.width, ctx.height);
  m_viewportWidth = ctx.width;
  m_viewportHeight = ctx.height;

  Log(LogLevel::Trace, "GL context sucessfully created. Version: {}\n", m_name.c_str());

  return BackendStatus::Success;
}

void OpenGLRenderer::Shutdown() {
  /* Runs twice on a normal exit - once from Application::Run, once from the destructor - so
   * everything here is guarded on the context and clears it on the way out. */
  if (!m_context) return;

  for (auto &entry : m_textures)
    if (entry.second) glDeleteTextures(1, &entry.second);
  m_textures.clear();

  if (m_ibo) glDeleteBuffers(1, &m_ibo);
  if (m_vbo) glDeleteBuffers(1, &m_vbo);
  if (m_vao) glDeleteVertexArrays(1, &m_vao);
  if (m_program) glDeleteProgram(m_program);
  m_ibo = m_vbo = m_vao = m_program = 0;

  DetachAllRenderables();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();

  SDL_GL_DestroyContext(static_cast<SDL_GLContext>(m_context));
  m_context = nullptr;
  m_window = nullptr;
}

/*!
 * First thing in the frame, before ImGui::NewFrame and before any behavior runs.
 *
 * The depth buffer is cleared but GL_DEPTH_TEST is never enabled: draw order comes entirely from
 * the band/layer sort in RenderBackend::RenderFrame. Enabling depth is the first thing to do here
 * if that ever changes, alongside SDL_GL_DEPTH_SIZE at window creation.
 *
 * TODO m_backgroundColor is declared in gfx.h but not read here; the clear is hardcoded black.
 */
void OpenGLRenderer::BeginFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL3_NewFrame();

  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/*!
 * Last thing in the frame, after every Draw and after ImGui::Render. ImGui goes straight to the
 * default framebuffer here, so anything that renders into an FBO must resolve *before* this or
 * the HUD ends up underneath it.
 */
void OpenGLRenderer::EndFrame() {
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow(static_cast<SDL_Window *>(m_window));
}

/* TODO On a HiDPI display the drawable is larger than the window, so this wants
 * SDL_GetWindowSizeInPixels rather than the logical size the resize event carries. */
void OpenGLRenderer::OnResize(int width, int height) {
  if (!m_context) return;
  glViewport(0, 0, width, height);
  m_viewportWidth = width;
  m_viewportHeight = height;
}

const char *OpenGLRenderer::GetName() const { return m_name.c_str(); }

void *OpenGLRenderer::GetNativeHandle() const { return m_context; }

#pragma region the built-in pipeline

/* One program covers the whole draw vocabulary. Materials and shaders-as-assets are deliberately
 * out of scope, so there is nothing here to select between: a command is coloured, optionally
 * textured, and either in world space or in window pixels.
 *
 * Interface between the two stages and the C++ side, in one place so it is easy to extend:
 *
 *   attribute location 0  vec3 aPosition   <- DrawVertex::position   (set up in BuildPipeline)
 *   attribute location 1  vec4 aColor      <- DrawVertex::color, straight alpha
 *   attribute location 2  vec2 aTexCoord   <- DrawVertex::texCoord
 *
 *   uniform uViewProjection <- RenderBackend::m_viewProjection   (set in Draw)
 *   uniform uModel          <- DrawCommand::transform
 *   uniform uScreenSpace    <- RENDERABLE_SCREEN_SPACE
 *   uniform uViewport       <- m_viewportWidth/Height, in pixels
 *   uniform uUseTexture     <- whether ResolveTexture found something
 *   uniform uTexture        <- never set; defaults to unit 0, the only unit bound
 *
 * Adding a uniform: declare it below, fetch its location in BuildPipeline into a new m_u* member
 * in gfx.h, set it in Draw. Adding an attribute also means a field on DrawVertex, which every
 * backend and every Collect shares.
 *
 * The `#version` line is not part of these bodies; CompileStage prepends it as a second source
 * string so GLSLVersionDirective stays the single authority for both this program and ImGui. */
namespace {

  constexpr const char *kVertexBody = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uViewProjection;
uniform mat4 uModel;
uniform int  uScreenSpace;
uniform vec2 uViewport;

out vec4 vColor;
out vec2 vTexCoord;

void main() {
  vColor = aColor;
  vTexCoord = aTexCoord;
  if (uScreenSpace != 0) {
    // Window pixels, top-left origin, straight to clip space.
    vec2 ndc = vec2(aPosition.x / uViewport.x * 2.0 - 1.0, 1.0 - aPosition.y / uViewport.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
  } else {
    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
  }
}
)";

  constexpr const char *kFragmentBody = R"(
in vec4 vColor;
in vec2 vTexCoord;

uniform int uUseTexture;
uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
  FragColor = uUseTexture != 0 ? texture(uTexture, vTexCoord) * vColor : vColor;
}
)";

  GLuint CompileStage(const GLenum _stage, const char *_directive, const char *_body) noexcept {
    const GLuint shader = glCreateShader(_stage);
    const char *sources[2] { _directive, _body };
    glShaderSource(shader, 2, sources, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
      char log[1024] {};
      glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
      ROSE_LOG_ERROR("GL shader compilation failed:\n{}\n", log);
      glDeleteShader(shader);
      return 0;
    }
    return shader;
  }

} // namespace

/*!
 * Compiles and links the one program, caches its uniform locations, and creates the VAO with a
 * single interleaved VBO plus an IBO. The vertex layout recorded in the VAO is the DrawVertex
 * struct verbatim, so the offsets come from offsetof and never need hand-maintaining.
 *
 * Called once from Init. Nothing here is per-frame; the buffers get their storage in Draw.
 */
bool OpenGLRenderer::BuildPipeline() noexcept {
  /* The requested version, matching what ImGui is told: a driver never hands back a context
   * older than what was asked for, so this always compiles. */
  const char *directive = GLSLVersionDirective(m_versionMajor, m_versionMinor);
  if (!directive) directive = "#version 330 core\n";

  const String vertexDirective = Format("{}\n", directive);
  const GLuint vertex = CompileStage(GL_VERTEX_SHADER, vertexDirective.c_str(), kVertexBody);
  if (!vertex) return false;

  const GLuint fragment = CompileStage(GL_FRAGMENT_SHADER, vertexDirective.c_str(), kFragmentBody);
  if (!fragment) {
    glDeleteShader(vertex);
    return false;
  }

  m_program = glCreateProgram();
  glAttachShader(m_program, vertex);
  glAttachShader(m_program, fragment);
  glLinkProgram(m_program);

  glDeleteShader(vertex);
  glDeleteShader(fragment);

  GLint linked = GL_FALSE;
  glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
  if (!linked) {
    char log[1024] {};
    glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
    ROSE_LOG_ERROR("GL program link failed:\n{}\n", log);
    glDeleteProgram(m_program);
    m_program = 0;
    return false;
  }

  m_uViewProjection = glGetUniformLocation(m_program, "uViewProjection");
  m_uModel = glGetUniformLocation(m_program, "uModel");
  m_uUseTexture = glGetUniformLocation(m_program, "uUseTexture");
  m_uScreenSpace = glGetUniformLocation(m_program, "uScreenSpace");
  m_uViewport = glGetUniformLocation(m_program, "uViewport");

  glGenVertexArrays(1, &m_vao);
  glGenBuffers(1, &m_vbo);
  glGenBuffers(1, &m_ibo);

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

  const auto stride = static_cast<GLsizei>(sizeof(DrawVertex));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(offsetof(DrawVertex, position)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(offsetof(DrawVertex, color)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(offsetof(DrawVertex, texCoord)));

  glBindVertexArray(0);
  return true;
}

/*!
 * TextureRegistry UUID -> GL texture name, uploaded on first sight and cached in m_textures until
 * Shutdown. There is no invalidation: a Surface that changes after its first upload is never
 * re-uploaded. Streamed or animated textures need a SubImage path or a registry hook first.
 *
 * @retval 0 for an untextured id or one the registry cannot serve; Draw then falls back to
 *         vertex colour rather than sampling an unbound unit.
 */
uint32_t OpenGLRenderer::ResolveTexture(const TextureID &_id) noexcept {
  if (_id == UUID::Invalid()) return 0;

  if (const auto it = m_textures.find(_id); it != m_textures.end()) return it->second;

  GLuint name = 0;
  const Surface *surface = TextureRegistry::Get().GetTexture(_id);
  if (surface && surface->IsValid() && surface->GetPixels()) {
    glGenTextures(1, &name);
    glBindTexture(GL_TEXTURE_2D, name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /* The registry normalises everything to ARGB32, which GL has no enum for. BGRA plus
     * UNSIGNED_INT_8_8_8_8_REV is the byte-exact match on little-endian, so no CPU swizzle. */
    glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->GetPitch() / 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, surface->GetWidth(), surface->GetHeight(), 0, GL_BGRA,
                 GL_UNSIGNED_INT_8_8_8_8_REV, surface->GetPixels());
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  m_textures.insert(_id, name); // cached even when 0, so a miss costs one lookup, not one a frame
  return name;
}

/*!
 * The one backend-specific operation. Called by RenderBackend::RenderFrame once per command, in
 * final draw order (opaque, transparent, overlay; then layer; then enrollment order).
 *
 * Deliberately stateless between calls: no batching, no state caching, one upload and one draw
 * call per command. The steps are:
 *   1. blend state from the Transparent flag
 *   2. bind program + VAO
 *   3. stream the vertices into the VBO (orphaning, see below)
 *   4. uniforms
 *   5. texture, if Textured and resolvable
 *   6. topology -> GL primitive mode
 *   7. indexed or non-indexed draw
 *
 * What is left dirty on return: program, blend enable/func, active unit, and the unit-0 texture.
 * Only the VAO is unbound. ImGui's backend saves and restores its own state so it is unaffected,
 * but any new GL code here should either restore what it touches or set it unconditionally at
 * the top, the way blend is.
 *
 * Depth: never tested. Screen-space commands write z = 0 and world-space ones whatever the
 * projection yields, but with GL_DEPTH_TEST off it only matters for ordering if that changes.
 */
void OpenGLRenderer::Draw(const DrawCommand &_cmd) {
  if (!m_program || _cmd.vertexCount == 0) return;

  const bool screenSpace = (_cmd.flags & RENDERABLE_SCREEN_SPACE) != 0;
  const bool blend = (_cmd.flags & RENDERABLE_TRANSPARENT) != 0;

  // Straight-alpha colours (DrawVertex::color), hence the classic SRC_ALPHA / ONE_MINUS pair for RGB.
  if (blend) {
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    glDisable(GL_BLEND);
  }

  glUseProgram(m_program);
  glBindVertexArray(m_vao);

  /* Orphan both buffers each draw rather than sub-updating: the driver hands back fresh storage
   * instead of stalling on the previous frame's. */
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(_cmd.vertexCount * sizeof(DrawVertex)), _cmd.vertices,
               GL_STREAM_DRAW);

  // ROSE matrices are row-major; GL reads column-major, hence the transpose.
  glUniformMatrix4fv(m_uViewProjection, 1, GL_TRUE, m_viewProjection.data.data());
  glUniformMatrix4fv(m_uModel, 1, GL_TRUE, _cmd.transform.data.data());
  glUniform1i(m_uScreenSpace, screenSpace ? 1 : 0);
  glUniform2f(m_uViewport, static_cast<float>(m_viewportWidth), static_cast<float>(m_viewportHeight));

  const uint32_t texture = (_cmd.flags & RENDERABLE_TEXTURED) ? ResolveTexture(_cmd.texture) : 0;
  glUniform1i(m_uUseTexture, texture ? 1 : 0);
  if (texture) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
  }

  GLenum mode = GL_TRIANGLES;
  switch (_cmd.topology) {
  case Topology::Points:
    mode = GL_POINTS;
    glPointSize(_cmd.pointSize);
    break;
  case Topology::Lines:
    mode = GL_LINES; // disjoint pairs, per Topology::Lines
    break;
  case Topology::Triangles:
    mode = GL_TRIANGLES;
    break;
  }

  if (_cmd.indices && _cmd.indexCount > 0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(_cmd.indexCount * sizeof(uint32_t)), _cmd.indices,
                 GL_STREAM_DRAW);
    glDrawElements(mode, static_cast<GLsizei>(_cmd.indexCount), GL_UNSIGNED_INT, nullptr);
  } else {
    glDrawArrays(mode, 0, static_cast<GLsizei>(_cmd.vertexCount));
  }

  glBindVertexArray(0);
}

#pragma endregion
