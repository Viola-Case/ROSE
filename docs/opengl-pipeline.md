# The OpenGL rendering pipeline

How a frame gets from a `Renderable` to the screen when the application runs with
`ApplicationFlag::OpenGL`. Read this before extending `OpenGLRenderer`; the last section lists the
places a new feature is most likely to plug in and the traps around each one.

Files:

| File | Role |
|---|---|
| `include/ROSE/Core/gfx.h` | the draw vocabulary (`DrawVertex`, `DrawCommand`, flags, `Topology`), `RenderBackend`, and the `OpenGLRenderer` declaration |
| `src/Core/gfx.cpp` | the backend-independent half: enrollment and the per-frame collect/sort/draw pass |
| `src/Core/openglrenderer.cpp` | the GL half: context, the one shader program, streaming buffers, texture cache, `Draw` |
| `src/Core/application.cpp` | backend selection, window creation, and the frame loop that calls into the backend |
| `src/Core/renderable.cpp` | the `Collect` implementations that produce `DrawCommand`s |

## 1. Where the backend sits

`RenderBackend` is split in two on purpose. Everything that is *not* GL-specific lives in the base
class and is shared with the SDL and software backends: which renderables exist, whether they are
enabled, what order they draw in. A backend implements only the lifecycle (`Init`, `Shutdown`,
`BeginFrame`, `EndFrame`, `OnResize`) plus exactly one drawing operation, `Draw(const DrawCommand &)`.

Consequence: a feature that changes *what* is drawn or *in what order* belongs in `gfx.cpp` or the
renderables, not in the GL backend. A feature that changes *how* a command hits the GPU belongs in
`openglrenderer.cpp`.

## 2. Lifetime

```
Application::Init
  ├─ flags & OpenGL           → windowFlags |= SDL_WINDOW_OPENGL; m_renderer = new OpenGLRenderer()
  ├─ ImGui::CreateContext
  ├─ Window::Create(...)      ← pixel-format GL attributes (depth size, MSAA) must be set BEFORE this
  └─ m_renderer->Init(ctx)    → OpenGLRenderer::Init  (fatal on anything but Success)

Application::Run  (per frame, see §3)

Application teardown
  ├─ m_renderer->Shutdown()   (explicit)
  └─ delete m_renderer        → ~OpenGLRenderer → Shutdown() again, guarded on m_context
```

`OpenGLRenderer::Init`, in order:

1. Reads the `SDL_Window *` out of `RenderBackendContext::window`. A window created without
   `SDL_WINDOW_OPENGL` fails at the next step; that is the usual cause of `ContextCreationFailed`.
2. Sets the three context attributes SDL reads at context creation: major, minor, core profile.
   Everything pixel-format related (`SDL_GL_DEPTH_SIZE`, `SDL_GL_DOUBLEBUFFER`, multisampling) is
   read at *window* creation and is inert here. Those belong in `Application::Init` next to
   `Window::Create`.
3. `SDL_GL_CreateContext`.
4. `gladLoadGL(SDL_GL_GetProcAddress)`. Nothing may touch a `gl*` symbol before this returns; they are
   null pointers until glad fills the table. ImGui's GL3 init calls `glGetString`, so it comes after.
5. Queries the version the driver actually handed back (asking for 4.5 usually yields 4.6) and builds
   `m_name` from it. This is only for logs.
6. ImGui: `ImGui_ImplSDL3_InitForOpenGL`, then `ImGui_ImplOpenGL3_Init(directive)` where `directive`
   is the GLSL `#version` line for the *requested* GL version (see `GLSLVersionDirective`). The
   requested version is used deliberately: a driver never returns an older context than requested,
   so it is always a version the context can compile.
7. Swap interval from `ctx.vsync`.
8. `BuildPipeline()`: compiles and links the one built-in program, caches its uniform locations,
   creates the VAO/VBO/IBO and fixes the vertex layout.
9. Initial `glViewport` and the cached viewport size, which the vertex shader needs for screen-space
   commands.

Every failure path after context creation destroys what it made and returns a `BackendStatus`.
`Application::Init` treats anything but `Success` as fatal, because a half-initialised backend leaves
ImGui's renderer impl unbound and the first `BeginFrame` asserts inside ImGui instead.

`Shutdown` runs twice on a normal exit (once explicitly, once from the destructor), so it is guarded
on `m_context` and nulls it on the way out. It deletes every cached texture, the three buffer
objects, the program, then detaches renderables, shuts ImGui down, and destroys the context.

## 3. One frame

`Application::Run` drives the backend in this fixed order every frame:

```
m_renderer->BeginFrame()            ImGui GL3 + SDL3 NewFrame; glClearColor(black); glClear(COLOR|DEPTH)
ImGui::NewFrame()
… event pump (SDL_EVENT_WINDOW_RESIZED → m_renderer->OnResize) …
m_currentScene->FrameUpdate()       behaviors run; renderables update whatever Collect will read
m_renderer->SetViewProjection(ResolveViewProjection())
m_renderer->RenderFrame()           the shared pass, §3.1, ending in N calls to OpenGLRenderer::Draw
ImGui::Render()
m_renderer->EndFrame()              ImGui_ImplOpenGL3_RenderDrawData; SDL_GL_SwapWindow
```

The ordering guarantees two things: a renderable's `Collect` sees the state its own `FrameUpdate`
just produced, and ImGui is drawn after all scene geometry so the HUD is always on top.

`ResolveViewProjection` returns the first enabled `Camera`'s world-to-clip matrix, or, with no
camera, a pixel-space orthographic matrix (top-left origin) that matches the `RENDERABLE_SCREEN_SPACE`
convention. A scene made only of screen-space geometry therefore works with no camera at all.

### 3.1 The shared pass: `RenderBackend::RenderFrame` (gfx.cpp)

1. Clear the two frame-scratch lists (`m_frameCommands`, `m_frameOrder`); capacity is kept.
2. For every enrolled, enabled renderable, in enrollment order: hand it a `RenderList` that appends
   onto `m_frameCommands`, call `Collect`, and record one `SortEntry` per command it added:
   `{ band, layer, sequence, index }`.
   - `band`: 0 opaque, 1 transparent (`RENDERABLE_TRANSPARENT`), 2 overlay (`RENDERABLE_OVERLAY`).
   - `layer`: `Renderable::GetLayer()`.
   - `sequence`: enrollment order, so ties are reproducible.
   - `index`: position in `m_frameCommands`. Indices, not pointers, because the list reallocates
     while collecting.
3. `stable_sort` by band, then layer, then sequence.
4. Call `Draw` once per entry.

`RenderList::Add` drops commands with no vertices, so `Draw` never sees an empty one.

Pointers inside a `DrawCommand` are non-owning and must stay valid until the *last* `Draw` of the
pass returns, not merely until that command's own `Draw` returns. The GL backend consumes them
immediately, but the contract exists for backends that defer, and renderables honour it by not
touching their vertex storage until the frame ends.

### 3.2 The GL draw: `OpenGLRenderer::Draw` (openglrenderer.cpp)

Per command, with no batching and no state caching:

1. Bail if the program failed to build or the command has no vertices.
2. Blend state from `RENDERABLE_TRANSPARENT`: enable with
   `glBlendFuncSeparate(SRC_ALPHA, ONE_MINUS_SRC_ALPHA, ONE, ONE_MINUS_SRC_ALPHA)`, else disable.
   `DrawVertex::color` is straight alpha, which is why the RGB factors are the classic pair.
3. Bind the program and VAO.
4. Upload vertices with `glBufferData(GL_STREAM_DRAW)` into the single VBO. Each upload orphans the
   previous storage instead of sub-updating it, so the driver hands back fresh memory rather than
   stalling on the buffer the GPU may still be reading.
5. Set uniforms: view-projection, model, screen-space flag, viewport size. ROSE matrices are
   row-major and GL wants column-major, so both matrix uploads pass `transpose = GL_TRUE`.
6. If `RENDERABLE_TEXTURED`, resolve the `TextureID` to a GL texture name through the cache (§4) and
   bind it on unit 0. `uUseTexture` is set from whether the resolution *succeeded*, so a textured
   command whose texture is missing draws with vertex colour only rather than sampling garbage.
7. Map `Topology` to `GL_POINTS` / `GL_LINES` / `GL_TRIANGLES`. Points also set `glPointSize`.
   `Lines` is disjoint segments, never a strip, by contract in `gfx.h`.
8. Indexed: upload indices into the IBO the same way and `glDrawElements(GL_UNSIGNED_INT)`.
   Non-indexed: `glDrawArrays`.
9. Unbind the VAO. The program, blend state, and unit-0 texture binding are left as they are.

### 3.3 The shader program

One program covers every command. There is nothing to select between because materials and shader
assets are out of scope for the backend; the `Material` / `Shader` classes in `gfx.h` are not wired
in yet.

Vertex layout (fixed by `BuildPipeline`, one interleaved `DrawVertex` per vertex):

| location | attribute | type | source |
|---|---|---|---|
| 0 | `aPosition` | `vec3` | `DrawVertex::position` |
| 1 | `aColor` | `vec4` | `DrawVertex::color` (straight RGBA, 0..1) |
| 2 | `aTexCoord` | `vec2` | `DrawVertex::texCoord` |

Uniforms (locations cached in `m_u*` after link):

| uniform | set from |
|---|---|
| `mat4 uViewProjection` | `RenderBackend::m_viewProjection` |
| `mat4 uModel` | `DrawCommand::transform` |
| `int uScreenSpace` | `RENDERABLE_SCREEN_SPACE` |
| `vec2 uViewport` | cached viewport size in pixels |
| `int uUseTexture` | whether the texture resolved |
| `sampler2D uTexture` | never set explicitly; defaults to unit 0, which is the only unit used |

Vertex stage: screen-space positions are window pixels with a top-left origin and are mapped
straight to NDC with `z = 0`, skipping both matrices. Otherwise `uViewProjection * uModel * pos`.
Fragment stage: `texture * vColor` when textured, else `vColor`.

The `#version` line is prepended as a separate source string (`glShaderSource` with two strings),
chosen by `GLSLVersionDirective` for the requested GL version and falling back to `330 core`. The
same function feeds ImGui's backend, so the two always agree. Static assertions pin the awkward
cases (3.2 maps to GLSL 150, not 320; anything below 3.2 has no core profile).

## 4. Texture cache: `ResolveTexture`

`TextureID` is a `TextureRegistry` UUID. The first time a UUID is seen, the registry's `Surface` is
uploaded as a `GL_RGBA8` texture with linear filtering and clamp-to-edge wrapping, and the GL name is
stored in `m_textures`. The registry normalises everything to ARGB32; on little-endian that is
byte-identical to `GL_BGRA` + `GL_UNSIGNED_INT_8_8_8_8_REV`, so there is no CPU swizzle.
`GL_UNPACK_ROW_LENGTH` is set from the surface pitch so padded rows upload correctly.

A miss (unknown UUID, invalid surface, no pixels) is cached as `0`, so it costs one hash lookup per
frame afterwards rather than a registry query. There is no invalidation: a surface that changes
after first upload is never re-uploaded, and nothing is evicted until `Shutdown`.

## 5. Resize

`SDL_EVENT_WINDOW_RESIZED` reaches `OnResize`, which updates `glViewport` and the cached viewport
size. The size comes from the event, which is the *logical* window size. On a HiDPI display the
drawable is larger; the fix, still a TODO in the source, is `SDL_GetWindowSizeInPixels`.

## 6. Extending it

The places new functionality is likely to land, and what to watch for at each.

**Depth testing.** Not enabled. `BeginFrame` clears `GL_DEPTH_BUFFER_BIT`, but `GL_DEPTH_TEST` is
never turned on, and the window is created with SDL's default depth size, which may be zero. Overlay
and transparent ordering work purely through the sort in `RenderFrame`. To add depth: set
`SDL_GL_DEPTH_SIZE` in `Application::Init` before `Window::Create`, `glEnable(GL_DEPTH_TEST)` in
`Init` or `BeginFrame`, and disable it (or `glDepthMask(GL_FALSE)`) for the transparent and overlay
bands inside `Draw`, which can read the band from the flags the same way `BandOf` in `gfx.cpp` does.
Screen-space commands write `z = 0` and will need the test off too.

**Clear colour.** `m_backgroundColor` exists in the header but `BeginFrame` hardcodes black. Wire
the member in, or remove it.

**Batching / instancing.** Every command is its own upload and draw call. A batcher would sit in
`Draw` (accumulate, flush on a state change or at `EndFrame`) and must respect the `DrawCommand`
lifetime contract, which `RenderFrame` already guarantees for the whole pass. Model matrices would
have to be applied on the CPU or moved into a per-instance attribute.

**More uniforms or attributes.** Add to `kVertexBody` / `kFragmentBody`, look the location up in
`BuildPipeline`, store it in a new `m_u*` member in `gfx.h`, set it in `Draw`. New attributes also
need a `glVertexAttribPointer` in `BuildPipeline` and a field in `DrawVertex`, which changes the
struct every backend and every `Collect` shares; the SDL and software backends read the same struct.

**Multiple programs / materials.** `Material`, `Shader`, `Uniform`, and `ShaderRegistry` in `gfx.h`
are declared but not implemented. `RenderBackend::GetUniforms` is the only hook that exists. Nothing
on `DrawCommand` names a material yet; that field, plus a program table keyed on it, is where this
would start. Keep `GLSLVersionDirective` as the single source of the `#version` line.

**Framebuffers / post-processing.** Draw into an FBO between `BeginFrame` and `EndFrame`, then
resolve to the default framebuffer *before* `ImGui_ImplOpenGL3_RenderDrawData` so the HUD stays
on top and at native resolution. `OnResize` will have to reallocate the attachments.

**Texture updates.** `ResolveTexture` never re-uploads. A `SubImage` path or an invalidation hook on
`TextureRegistry` is needed before animated or streamed surfaces work.

**GL debugging.** Nothing calls `glGetError` and no `KHR_debug` callback is installed. On a 4.3+
context, `glDebugMessageCallback` in `Init` is the cheapest way to find state errors while working on
any of the above; request the context with `SDL_GL_CONTEXT_DEBUG_FLAG` to get the useful messages.

**State assumptions `Draw` relies on.** The VAO is unbound at the end of every draw, but the program,
blend enable/func, active texture unit, and unit-0 texture are not restored. ImGui's GL3 backend saves
and restores everything it touches, so it does not interfere. Any new code that changes GL state
should either restore it or set it unconditionally at the top of `Draw`, as blend is now.
