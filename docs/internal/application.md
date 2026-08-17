# Application — internal reference

The top of the runtime: one `Application` owns the window, the render backend, the
scenes, and the frame loop. Everything a game does happens inside `Run()`.

Checked against the sources on **2026-08-16** (`master` @ `9e2683c` plus the
`ApplicationInitSettings` work in the tree). For what lives *under* an application,
see [`scene-object-behavior.md`](scene-object-behavior.md).

| Piece | Header | Source |
|---|---|---|
| `Application`, `ApplicationInitSettings`, `ApplicationFlag` | `include/ROSE/Core/application.h` | `src/Core/application.cpp` |
| `Window` | `include/ROSE/Core/window.h` | `src/Core/window.cpp` |
| `RenderBackend` and friends | `include/ROSE/Core/gfx.h` | `src/Core/gfx.cpp`, `softwarerenderer.cpp`, `openglrenderer.cpp` |

---

## 1. The shape of a `main()`

```cpp
int main() {
  // 1. Register behaviors. Must happen before any scene is parsed.
  BehaviorFactory &factory = BehaviorFactory::get();
  RoseRegisterCoreModule(factory);
  factory.Register(MakeBehavior<Paddle>, Paddle::TypeID(), "Game1");

  // 2. Describe the application. Nothing is created yet.
  ApplicationInitSettings settings { "Game 1" };
  settings.SetFlags(APPLICATION_SOFTWARE_RENDERER | APPLICATION_CONTROLLER_SUPPORT)
    .SetWindowSize(800, 600)
    .AddSceneFromFile("assets/game1scene1.json");

  // 3. Build it.
  Application app;
  if (const int err = app.Init(Move(settings))) return err;

  AttachImGui();      // only if this executable draws ImGui itself; see README.md

  // 4. Run until something calls Quit().
  app.Run();
  return 0;
}
```

**`Application` has no pre-init member functions, deliberately.** The constructor
takes nothing and does nothing but raise the Windows timer resolution; `Init` is the
only thing that reads configuration, and it reads it exclusively from the settings
object. If a knob has to be set before the window exists, it is a field on
`ApplicationInitSettings` — there is nowhere else to put it.

`Init` takes the settings by **rvalue** and moves the scenes out of them, so the
settings object is spent afterwards. Pass `Move(settings)` or a temporary.

An `Application` on the stack is fine and is what the examples do. It is
non-copyable and non-movable.

## 2. `ApplicationInitSettings`

Move-only (it owns `Scene`s). Every setter returns `*this`, so configuration chains.

| Field | Setter | Default | Consumed by |
|---|---|---|---|
| title | `SetTitle(StringView)` | `"Application"` | window caption, `GetTitle()` |
| organization | `SetOrganization(StringView)` | `""` | the vendor half of the per-user paths |
| window size | `SetWindowSize(Vec2<int>)` / `(int,int)` | `800 × 600` | `Window::Create`; ignored when headless |
| flags | `SetFlags(ApplicationFlags)`, `SetFlag(ApplicationFlag, bool)` | `APPLICATION_DEFAULT` | §3 |
| vsync | `SetVSync(bool)` | `true` | `RenderBackendContext::vsync` |
| frame cap | `SetTargetFrameRate(uint32_t)` | `0` (uncapped) | the sleep at the bottom of `Run()` |
| existing window | `SetExistingWindow(void *)` | `nullptr` | **nothing yet** — see §8.1 |
| scenes | `AddScene(Scene&&)`, `AddSceneFromFile(StringView)`, `SetScenes(List<Scene>&&)` | empty | moved into `Application::m_scenes` |

Matching `GetX()` accessors exist for all of them; `Application` is a `friend` and
reads the members directly.

`AddSceneFromFile` reads the file whole and hands it to `Scene::FromJSONString`. A
file that will not open is logged and **skipped** — the call still returns `*this`,
so check `GetScenes().size()` if you need to know. It is the only reason
`<fstream>` appears in `application.cpp`, and it exists because every example was
otherwise open-coding the same six lines of `ifstream`/`stringstream`.

**The first scene added is the one the application starts on.** There is no scene
switching API yet; `m_currentScene` is set once by `Init` and never moves.

The special member functions are all defaulted **out of line** in
`application.cpp`. That is not decoration: `application.h` only forward-declares
`Scene`, and defining them inline would instantiate `List<Scene>`'s destructor
against an incomplete type in every TU that includes the header. `application.h`
must not include `scene.h` for the same reason — `scene.h` holds a
`List<UniquePtr<Object>>` and `Object` is incomplete there, so pulling it into a TU
that never includes `object.h` (e.g. `gfx.cpp`) produces
`-Wdelete-incomplete` on `UniquePtr<Object>::~UniquePtr`.

## 3. Flags

`ApplicationFlags` is a `uint64_t` bitmask. `ApplicationFlag` is the *bit index*
enum — the two are easy to confuse and the header says so. Use the
`APPLICATION_*` constants for the mask and `ApplicationFlag::X` for
`SetFlag`/`GetFlag`.

```cpp
constexpr ApplicationFlags APPLICATION_DEFAULT = APPLICATION_CONTROLLER_SUPPORT
                                               | APPLICATION_SOFTWARE_RENDERER;
```

| Flag | Bit | Read by the engine? |
|---|---|---|
| `Headless` | 0 | ✅ skips window creation and the renderer |
| `SoftwareRenderer` | 4 | ✅ selects `SoftwareRenderer`; see §6 for what "off" does |
| `ControllerSupport` | 5 | ✅ adds `SDL_INIT_GAMEPAD \| JOYSTICK \| HAPTIC` |
| `Debug` | 63 | ⚠️ *set* by `Init` under `_DEBUG`, never read |
| `NoRenderer`, `Server`, `Light`, `NoSound`, `Vulkan`, `OpenGL`, `DirectX9/11/12`, `Metal` | 1-3, 6-12 | ❌ inert |

Only three flags do anything today. `NoSound` in particular does not suppress
`SDL_INIT_AUDIO`, which `Init` always requests.

`ApplicationFlag`'s constructor `throw`s on an out-of-range value. It is
`constexpr`-only in practice, which is what keeps it inside the no-exceptions rule
(see [`conventions.md`](conventions.md) §5).

## 4. `Init` — what actually happens, in order

`src/Core/application.cpp`. Returns `0` on success, negative on failure.

```
1.  copy/move every field out of the settings         (title, org, size, flags,
                                                       fps cap, vsync, scenes)
2.  |= APPLICATION_DEBUG under _DEBUG
3.  warn if an existing window handle was supplied     (unimplemented, §8.1)
4.  SDL_VERSION vs SDL_GetVersion()                    -> -2 on mismatch
5.  SDL_Init(VIDEO | AUDIO | EVENTS [| GAMEPAD…])      -> -3 on failure
6.  InputSystem::GetInstance().Init()                  (an empty function today)
7.  pick window flags / construct the render backend
8.  ImGui::CreateContext() + StyleColorsDark()         (unconditional, even headless)
9.  Window::Create(title, size, flags)                 -> -4 if invalid
10. m_renderer->Init(RenderBackendContext{ handle, w, h, vsync })
11. m_currentScene = scenes.empty() ? nullptr : begin()
12. Bind() every scene to this application
```

Two things worth knowing about step 9: the window is created **hidden**
(`SDL_WINDOW_HIDDEN`) and `Run()` shows it, so nothing flashes on screen while the
renderer comes up. And no `SDL_WINDOW_RESIZABLE` is requested, so the resize
handling in the loop only fires for programmatic size changes.

`Init` is not guarded against being called twice.

## 5. `Run` — the frame loop

```
Run()
  m_window->Show()
  while (!m_shouldClose):
    memcpy(m_keyStatePrevious, m_keyState, 256)     // InputSystem previous-frame snapshot
    m_renderer->BeginFrame()
    ImGui::NewFrame()
    if (m_currentScene && scene changed) m_currentScene->OnStart()
    while (SDL_PollEvent):
      SDL_EVENT_QUIT            -> m_shouldClose = true
      SDL_EVENT_WINDOW_RESIZED  -> Window::OnResized + RenderBackend::OnResize
      SDL_EVENT_WINDOW_MOVED    -> Window::OnMoved
      ImGui_ImplSDL3_ProcessEvent
    Time::dT = <seconds since the previous iteration>
    if (m_currentScene) m_currentScene->FrameUpdate()
    ImGui::Render()
    m_renderer->EndFrame()                          // includes present
    <frame pacing>
  m_renderer->Shutdown()
```

**Frame pacing.** With `m_targetFrameRate == 0` the loop sleeps a flat 1 ms so it
does not spin a core flat. Otherwise it sleeps whatever is left of the
`1 / fps` budget measured from the top of the frame. `SetTargetFrameRate` is also
available post-`Init` and takes effect on the next frame. This is independent of
vsync — with both on, whichever limit is tighter wins.

**Every scene call is null-guarded.** An application initialized with no scenes at
all is legal (a headless or server run has nothing to update), and `m_currentScene`
is `nullptr` in that case rather than pointing into an empty `List`'s storage.

`Run()` is re-entrancy-guarded by `m_isRunning`, not one-shot: its doc comment
claims it "terminates if called again", but it simply returns. After a normal exit
`m_shouldClose` is still `true`, so a second call skips the loop body entirely and
goes straight to a second `m_renderer->Shutdown()`.

`Quit()` just sets `m_shouldClose`; the current frame finishes first.

`InputSystem::Poll()` exists and does exactly the `memcpy` at the top of the loop,
but `Run()` open-codes it rather than calling it, and `Poll` has **no call sites
anywhere in the tree**.

## 6. Renderer selection

```cpp
if (!GetFlag(Headless)) {
  if (!GetFlag(SoftwareRenderer)) {
    windowFlags |= (1 ? SDL_WINDOW_VULKAN : SDL_WINDOW_OPENGL);   // literal `1`
  } else {
    m_renderer = new SoftwareRenderer();
  }
}
```

`SoftwareRenderer` is the only backend `Application` ever constructs. It is not
really a software rasteriser — it is SDL's own renderer, asked for
`"vulkan, direct3d11, opengl, gpu, software"` in that order, so it is normally
hardware-accelerated. It owns the ImGui SDL3 + SDLRenderer3 backends and honours
`ctx.vsync` through `SDL_SetRenderVSync`.

**Clearing the `SoftwareRenderer` flag leaves the application with no renderer at
all.** The `else` branch only sets a window flag; `m_renderer` stays `nullptr`, no
ImGui platform backend is initialised, and `ImGui::NewFrame()` then runs every frame
without a matching backend `NewFrame`. `OpenGLRenderer` is fully implemented in
`openglrenderer.cpp` and honours `ctx.vsync` via `SDL_GL_SetSwapInterval`, but
nothing in the tree ever constructs one.

## 7. Post-`Init` API

```cpp
const char *GetTitle() const noexcept;
const char *GetOrganization() const noexcept;

Window       *GetWindow() noexcept;          // nullptr when headless — always check
const Window *GetWindow() const noexcept;

Scene *GetCurrentScene() noexcept;           // nullptr before Init, or if no scenes
const List<Scene> &GetScenes() noexcept;

bool     GetFlag(ApplicationFlag) const noexcept;
uint32_t GetTargetFrameRate() const noexcept;
void     SetTargetFrameRate(uint32_t) noexcept;

void SetWindowSize(math::Vec2<int>) noexcept;    // live resize; warns if there is no window
void SetWindowSize(int, int) noexcept;

void Quit() noexcept;
```

`SetWindowSize` is a runtime resize, not a pre-init setter — it pushes to the
backend and to the renderer. The *initial* size comes from the settings.

## 8. Sharp edges

1. **`SetExistingWindow` is stored and ignored.** `Window` can only `Create`, so
   `Init` logs a warning and makes its own window anyway. It needs a
   `Window::Adopt(void *)` over `SDL_CreateWindowWithProperties` before the editor
   can embed a game viewport. There is an `@todo` on the setter.

2. **`Application::LoadModule(StringView)` is declared and never defined.** It is
   also pre-init by nature — a module has to load before any scene naming its
   behaviors is deserialized — so it contradicts the "no pre-init member functions"
   rule it sits next to. Once dynamic loading exists it should become a module list
   on `ApplicationInitSettings` that `Init` walks. Calling it today is a link error.

3. **`UniquePtr<SceneManager> m_manager` is never assigned**, and `SceneManager`
   (`scene.h`) is an empty class with nothing but `friend class Application`.

4. **Ten of the fourteen flags are inert** (§3), and `Debug` is set but never read.
   Only `Headless`, `SoftwareRenderer` and `ControllerSupport` change anything.

5. **Scene switching would re-run `OnStart()`.** The guard in `Run()` is a
   `static Scene *lastScene`, so returning to a previously active scene replays
   `OnCreate()` and `OnStart()` over behaviors that already ran them. Only bites at
   2+ scenes, and there is no switching API yet anyway. Same item as
   [`scene-object-behavior.md`](scene-object-behavior.md) §8.10.

6. **The `Orbits` example does not run.** `examples/orbits/` contains only
   `main.cpp` — the behavior its scene names
   (`f69e87e51985f92b-cd8aa43477659a22`) and `examples/orbits/assets/` are both
   absent from the repo, so `rose_deploy_assets` points at nothing and the loader
   dereferences a null factory result (`scene-object-behavior.md` §8.2). Its
   `main.cpp` compiles and is current; the rest of the example needs restoring.
