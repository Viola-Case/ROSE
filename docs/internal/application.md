# Application — internal reference

The top of the runtime: one `Application` owns the window, the render backend, the
scenes, and the frame loop. Everything a game does happens inside `Run()`.

Checked against the sources on **2026-09-02** (`master` @ `de3eafa`); first written
against `9e2683c` (**2026-08-16**). For what lives *under* an application, see
[`scene-object-behavior.md`](scene-object-behavior.md).

| Piece | Header | Source |
|---|---|---|
| `Init()` / `InitStatus` | `include/ROSE/Core/init.h` | `src/Core/init.cpp` |
| `Application`, `ApplicationInitSettings`, `ApplicationFlag` | `include/ROSE/Core/application.h` | `src/Core/application.cpp` |
| `Window` | `include/ROSE/Core/window.h` | `src/Core/window.cpp` |
| `RenderBackend` and friends | `include/ROSE/Core/gfx.h` | `src/Core/gfx.cpp`, `sdlrenderer.cpp`, `softwarerenderer.cpp`, `openglrenderer.cpp` |

---

## 1. The shape of a `main()`

```cpp
int main() {
  // 1. Bring the engine up. This also registers the Core module's behaviors,
  //    so main() never calls RoseRegisterCoreModule itself.
  if (auto i = Init(); i != InitStatus::Success) return static_cast<int>(i);

  // 2. Register this module's behaviors. Must happen before any scene is parsed,
  //    which now means before AddSceneFromFile, not merely before Init().
  {
    BehaviorFactory &factory = BehaviorFactory::Get();
    Pair<FactoryFn, UUID> fns[] {
      ROSE_BEHAVIOR_REGISTRY_PAIR(AppCloser), ROSE_BEHAVIOR_REGISTRY_PAIR(Paddle),
    };
    for (const auto &p : fns) factory.Register(p.first, p.second, "Game1");
  }

  // 3. Describe the application. Nothing is created yet.
  ApplicationInitSettings settings { "Game 1" };
  settings.SetFlags(APPLICATION_SDL_RENDERER)
    .SetWindowSize(800, 600)
    .AddSceneFromFile("assets/game1scene1.json")
    .SetVSync(true);

  // 4. Build it.
  Application app;
  if (const int err = app.Init(Move(settings))) return err;

  AttachImGui();      // only if this executable draws ImGui itself; see README.md

  // 5. Run until something calls Quit().
  app.Run();
  return 0;
}
```

**`ROSE::Init()` comes first, before anything else.** It is a free function in
`ROSE/Core/init.h` returning an `InitStatus` enum, and it is what stands up the
process-wide singletons — including calling `RoseRegisterCoreModule(BehaviorFactory::Get())`
for you (`init.cpp:30`). Game code registers only its own behaviors; no example in
the tree calls `RoseRegisterCoreModule` directly. `ROSE_BEHAVIOR_REGISTRY_PAIR(T)`
in `factory.h` expands to `{MakeBehavior<T>, T::TypeID()}` and is how every example
builds its registration array.

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
constexpr ApplicationFlags APPLICATION_DEFAULT = APPLICATION_SDL_RENDERER;
```

There are **fourteen** flags. This is the whole list — anything not here does not
exist, whatever an older example may pass:

| Flag | Bit | Constant | Read by the engine? |
|---|---|---|---|
| `Headless` | 0 | `APPLICATION_HEADLESS` | ✅ skips window creation and the renderer |
| `NoRenderer` | 1 | `APPLICATION_NO_RENDERER` | ✅ skips the renderer, keeps the window |
| `Server` | 2 | `APPLICATION_SERVER` | ❌ inert |
| `Light` | 3 | `APPLICATION_LIGHTWEIGHT` | ❌ inert |
| `SoftwareRenderer` | 4 | `APPLICATION_SOFTWARE_RENDERER` | ✅ selects the real rasterizer; see §6 |
| `LowPerformance` | 5 | `APPLICATION_LOW_PERFORMANCE` | ⚠️ read, but unreachably — see §8.7 |
| `SDLRenderer` | 6 | `APPLICATION_SDL_RENDERER` | ✅ selects SDL's 2D renderer (the default) |
| `Vulkan` | 7 | `APPLICATION_VULKAN` | ⚠️ warns and falls back to `SDLRenderer` |
| `OpenGL` | 8 | `APPLICATION_OPENGL` | ✅ selects `OpenGLRenderer` and adds `SDL_WINDOW_OPENGL` |
| `DirectX9` | 9 | `APPLICATION_DIRECTX9` | ⚠️ warns and falls back |
| `Metal` | 10 | `APPLICATION_METAL` | ⚠️ warns and falls back |
| `DirectX11` | 11 | `APPLICATION_DIRECTX11` | ⚠️ warns and falls back |
| `DirectX12` | 12 | `APPLICATION_DIRECTX12` | ⚠️ warns and falls back |
| `Debug` | 63 | `APPLICATION_DEBUG` | ⚠️ *set* by `Init` under `_DEBUG`, never read |

> **There is no `ControllerSupport` flag and no `NoSound` flag.** Earlier revisions
> of this file listed both. Gamepad, joystick and haptic subsystems are initialised
> unconditionally by `ROSE::Init()` (`SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK |
> SDL_INIT_HAPTIC`), and so is audio — nothing suppresses `SDL_INIT_AUDIO`. Bit 5 is
> `LowPerformance`, not controller support.

So: five flags genuinely steer behaviour (`Headless`, `NoRenderer`,
`SoftwareRenderer`, `SDLRenderer`, `OpenGL`), five warn and fall back, and four —
`Server`, `Light`, `LowPerformance`, `Debug` — do nothing you can observe.

`ApplicationFlag`'s constructor `throw`s on an out-of-range value. It is
`constexpr`-only in practice, which is what keeps it inside the no-exceptions rule
(see [`conventions.md`](conventions.md) §5).

## 4. `Init` — what actually happens, in order

Startup is split in two now. `ROSE::Init()` (`src/Core/init.cpp`, returns an
`InitStatus`) brings the process up, and `Application::Init` builds one application
on top of it. **`ROSE::Init()` must run first**; nothing checks that it did.

```
ROSE::Init()                                             src/Core/init.cpp
1.  SDL_VERSION vs SDL_GetVersion()      -> InitStatus::SDLVersionMismatch
2.  SDL_Init(VIDEO | AUDIO | EVENTS | GAMEPAD | JOYSTICK | HAPTIC)
                                         -> InitStatus::SDLInitFailed
3.  InputSystem::GetInstance().Init()    (an empty function today)
4.  RoseRegisterCoreModule(BehaviorFactory::Get())
                                         -> InitStatus::Success
```

`Application::Init` returns `0` on success, negative on failure. It has two
overloads — one taking `ApplicationInitSettings&`, one taking `&&` — and the lvalue
one just moves and forwards to the other, so the settings object is spent either
way.

```
Application::Init(ApplicationInitSettings&&)             src/Core/application.cpp
1.  move every field out of the settings              (title, org, size, flags,
                                                       fps cap, vsync, scenes)
2.  |= APPLICATION_DEBUG under _DEBUG
3.  warn if an existing window handle was supplied     (unimplemented, §8.1)
4.  pick window flags / construct the render backend   (§6)
5.  IMGUI_CHECKVERSION, ImGui::CreateContext, StyleColorsDark
                                                       (unconditional, even headless)
6.  Window::Create(title, size, flags)                 -> -4 if invalid
                                                       (skipped when Headless)
7.  m_renderer->Init(RenderBackendContext{ handle, w, h, vsync })
                                                       -> -5 on any non-Success
                                                          BackendStatus
8.  m_currentScene = scenes.empty() ? nullptr : begin()
9.  Bind() every scene to this application
```

Step 7 is deliberately not ignorable: a backend that failed to initialise leaves
ImGui's renderer impl unbound, and the first `BeginFrame` then trips an assert
inside ImGui rather than reporting anything useful at the call site.

Two things worth knowing about step 6: the window is created **hidden**
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
    m_renderer->SetViewProjection(ResolveViewProjection())  // active Camera, or a pixel ortho
    m_renderer->RenderFrame()                               // Collect -> order -> Draw
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

**Every scene call is null-guarded**, and so is every window and renderer call —
`m_window->Show()`, `BeginFrame`, `SetViewProjection`, `RenderFrame`, `EndFrame`
and `Shutdown` all sit behind `if (m_renderer)`. A headless run therefore executes
the whole loop, ImGui included, and simply draws nothing. An application
initialized with no scenes at all is equally legal, and `m_currentScene` is
`nullptr` in that case rather than pointing into an empty `List`'s storage.

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
if (!GetFlag(Headless) && !GetFlag(NoRenderer)) {
  if (GetFlag(OpenGL)) {
    windowFlags |= SDL_WINDOW_OPENGL;
    m_renderer = new OpenGLRenderer();
  } else if (GetFlag(SoftwareRenderer)) {
    m_renderer = new SoftwareRenderer();
  } else {
    // Vulkan/Metal/DirectX* warn and land here.
    m_renderer = new SDLRenderer();
  }
}
```

There are three backends, and all three are reachable:

| Class | Flag | What it is |
|---|---|---|
| `SDLRenderer` | `APPLICATION_SDL_RENDERER` (default) | SDL's own 2D renderer, asked for `"vulkan,direct3d11,opengl,gpu,software"` in that order, so normally hardware-accelerated. Owns the ImGui SDL3 + SDLRenderer3 backends; honours `ctx.vsync` via `SDL_SetRenderVSync`. |
| `SoftwareRenderer` | `APPLICATION_SOFTWARE_RENDERER` | A real rasterizer. Owns an ARGB8888 framebuffer, draws into plain memory, and blits once a frame through a streaming `SDL_Texture`. Its internal resolution is decoupled from the window — construct it with a fixed size, or call `SetInternalResolution`, and the present stretches with nearest-neighbour filtering. ImGui draws *after* the blit, so the HUD stays crisp over a chunky world. |
| `OpenGLRenderer` | `APPLICATION_OPENGL` | GL 4.5 core by default, one built-in shader program covering the whole draw vocabulary. |

> **Renamed.** `SoftwareRenderer` used to be the SDL-renderer class, despite its name —
> it asked SDL for a hardware driver first and `"software"` only as a last resort that
> never fired. That class is now `SDLRenderer` (`src/Core/sdlrenderer.cpp`), and the
> name `SoftwareRenderer` belongs to the actual rasterizer.
>
> **This is a behaviour change, not a compile error.** `APPLICATION_DEFAULT` moved from
> `APPLICATION_SOFTWARE_RENDERER` to `APPLICATION_SDL_RENDERER`, so code that passed
> `APPLICATION_SOFTWARE_RENDERER` explicitly and expected SDL's renderer now silently
> gets the rasterizer. Anything still calling SDL directly (game1's `Ball`/`Paddle`)
> must say `APPLICATION_SDL_RENDERER`.

A flag naming a backend nobody has written — `Vulkan`, `Metal`, `DirectX9/11/12` —
logs a warning and falls back to `SDLRenderer`. It used to leave `m_renderer` null,
with no ImGui platform backend initialised and `ImGui::NewFrame()` running every frame
without a matching backend `NewFrame`.

The SDL_Renderer creation, vsync and ImGui plumbing shared by `SDLRenderer` and
`SoftwareRenderer` lives in the internal `src/Core/sdlpresenter.h`, by composition —
the two backends are free to diverge.

## 6a. The render pass

`RenderBackend` carries the whole frame apart from one method. A backend implements
the lifecycle plus `Draw(const DrawCommand &)`; enrollment, ordering and the enabled
check are shared in `src/Core/gfx.cpp`. That is the point: **adding a backend touches
no behavior.**

- `Renderable::OnCreate` calls `Enroll`, `OnDestroy` calls `Withdraw`. `Enroll` is
  idempotent, because a scene switch replays `OnCreate` (see §9.5).
- `RenderFrame` collects a `DrawCommand` list from every *enabled* renderable, orders
  it — opaque, then transparent, then overlay; within a band by `GetLayer()`; ties by
  enrollment order — and draws it. Commands stay alive for the whole pass, so a
  backend may defer.
- `Application::Run` calls `SetViewProjection(ResolveViewProjection())` then
  `RenderFrame()`, between `Scene::FrameUpdate()` and `ImGui::Render()`.
- `GetRenderBackend()` is how a `Renderable` finds somewhere to enroll. It hands back
  the abstract base, never a concrete backend.
- `~RenderBackend` nulls every enrolled renderable's back-pointer. Ownership runs the
  wrong way at teardown — `~Application` deletes the backend in its body but `m_scenes`
  dies afterwards, as members — so without this every `~Renderable` would touch a freed
  backend.

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

4. **Four of the fourteen flags do nothing observable** (§3): `Server`, `Light`,
   `LowPerformance` (§8.7) and `Debug`, which `Init` sets under `_DEBUG` and nothing
   reads. Five more name a backend nobody has written and warn before falling back
   (§6). There is no `NoSound` flag, and nothing suppresses `SDL_INIT_AUDIO` —
   `ROSE::Init()` always requests it.

5. **Scene switching would re-run `OnStart()`.** The guard in `Run()` is a
   `static Scene *lastScene`, so returning to a previously active scene replays
   `OnCreate()` and `OnStart()` over behaviors that already ran them. Only bites at
   2+ scenes, and there is no switching API yet anyway. Same item as
   [`scene-object-behavior.md`](scene-object-behavior.md) §8.10.

6. ~~**The `Orbits` example does not run.**~~ — FIXED. It was missing both its
   behaviors and its `assets/` directory. Both are present now:
   `examples/orbits/` has `closer.h`, `pointcloud.{h,cpp}`, `trail.{h,cpp}`,
   `trailrenderer.{h,cpp}` and `assets/orbits.json`, and the two type IDs the
   scene names resolve to `PointCloud` and `Closer`, registered under the module
   name `"Orbits"`. It is now the best example of a backend-agnostic
   `Renderable` — `PointCloud` includes no SDL header and resolves no renderer, so
   the same demo runs on all three backends unchanged.

7. **`APPLICATION_LOW_PERFORMANCE` can never be observed.** The only read of it is
   in the *constructor*:

   ```cpp
   Application::Application() noexcept {
     if (!GetFlag(ApplicationFlag::LowPerformance)) timeBeginPeriod(1);   // Windows only
   }
   ```

   `m_flags` is `{ 0 }` until `Init` copies it out of the settings, and the settings
   object is not reachable from the constructor — so the test always sees `false`
   and `timeBeginPeriod(1)` always runs, however the caller sets the flag. This is
   the one knob that genuinely has to be decided before `Init`, which makes it the
   counter-example to the "no pre-init member functions" rule rather than a
   violation of it: the fix is for `Init` to raise the timer resolution after
   reading the flags, not for `Application` to grow a setter.
