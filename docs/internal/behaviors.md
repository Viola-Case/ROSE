# Behavior catalogue

Every type deriving from `ROSE::Behavior` in the tree as of **2026-09-02**
(`master` @ `de3eafa`), with its type ID and what it actually does. For the
lifecycle, factory, and JSON contract these all plug into, see
[`scene-object-behavior.md`](scene-object-behavior.md).

"Registered" means the type is wired into a `BehaviorFactory` at startup and can
therefore appear in a scene JSON file. Unregistered types can only be installed in
C++, through `Object::CreateBehavior<T>()` — which itself refuses an unregistered
type, so in practice registration is the only way in.

---

## Core (`ROSE_Core`)

Registered by `RoseRegisterCoreModule` in `src/Core/factory.cpp`, which
`ROSE::Init()` calls for you — game code never calls it directly.

| Behavior | Type ID | Header | Registered | State |
|---|---|---|---|---|
| `Camera` | `98b16c050e659798-2ba97b3cd1a9dd7c` | `Core/camera.h` | ✅ | Data + matrices |
| `Motion` | `ab0a57d02d8e9fde-462c4cdfe26597d3` | `Core/motion.h` | ✅ | **Implemented** |
| `AudioSource` | `8448e5b94997ad4d-ccee5b44f06598a2` | `Core/audiosource.h` | ✅ | Empty stub |
| `MeshRenderable` | `9cfaac61886d5e2e-9e75d84332659afc` | `Core/renderable.h` | ✅ | **Implemented** |
| `SpriteRenderable` | `32c5294ce2e000fd-271098999265987f` | `Core/renderable.h` | ✅ | **Implemented** |
| `ShapeRenderable` | `4e503b30bef7e6ef-a5250bb2d5659727` | `Core/renderable.h` | ✅ | **Implemented** |
| `GeometryRenderable` | `1f6b0c7d94a3e582-b0d47e2c3165a91d` | `Core/renderable.h` | ✅ | **Implemented** |
| `UIRenderable` | `cb497ab81ab3dd7b-85272b1b21659b69` | `Core/renderable.h` | ✅ | Base; draws nothing |
| `ImageUI` | `03552ebc7a5d6dea-0aecc877e265992b` | `Core/renderable.h` | ✅ | **Implemented** |
| `TextUI` | `9f24e65aa0a93483-992866975b659b59` | `Core/renderable.h` | ✅ | Draws nothing yet |
| `Renderable` | *(none)* | `Core/renderable.h` | — | Abstract base |
| `Collider` | `7edb97ba340571cc-d568181d0c659920` | `Core/collider.h` | ❌ | Empty stub |
| `UI` | `5be3e81fa226cdc4-c8105a5b51659aa0` | `Core/gui.h` | ❌ | Declared, **unimplemented** |

> **Changed.** The base `Renderable` used to carry a type ID of its own and was
> listed here as registered. It no longer has one — it is abstract (`Collect` is
> pure virtual) and cannot be minted by the factory. What is registered is the
> seven concrete subclasses above, each with its own ID. A scene file naming the
> old base ID (`0f01169dcb6855ae-daead65ffc659aec`) will not load; that ID no
> longer appears anywhere in the tree.

### `Motion`
The only non-`Renderable` Core behavior with a working `FrameUpdate`. Integrates the
owning object's `Transform` from four state vectors — velocity `m_drdt`, angular
velocity `m_dTdt`, acceleration `m_d2rdt2`, angular acceleration `m_d2Tdt2` — using
**semi-implicit Euler**: rates advance first, then position and rotation integrate
against the end-of-frame velocities, which keeps it from pumping energy in when
acceleration is nonzero. Angular velocity is axis-times-rate: direction is the axis,
magnitude the rate in rad/s, applied as a left-composed `Quatd::AxisAngle` delta
(world space, to match position) and re-normalized every frame so accumulated
rounding does not show up as the object slowly scaling.

Public setters/getters for all four vectors; the getters return non-const references,
which is how `Ball` flips velocity components in place on a bounce and how the cube
example's `RotateFunny` composes a rotation onto the angular velocity each frame.

`Unpack` reads `drdt` and `dTdt` as `Vec3d`. The accelerations are not deserializable
— set them from code.

### `Camera`
Carries `m_aspectRatio` (`Vec2<int16_t>`), `m_focalLength` (millimeters, default 30),
`m_orthographic`, `m_near`/`m_far`, and `m_orthographicSize`. `Unpack` reads
`focalLength`, `orthographic`, `orthographicSize`, `near` and `far`. Aspect ratio is
still declared but not unpacked — it belongs to the viewport, and
`GetViewProjection(aspect)` takes it as an argument instead.

`GetViewProjection`, `GetView` and `GetProjection` build the matrices out of
`math::Perspective` / `math::Orthographic` and the owning object's transform;
`Application::ResolveViewProjection` picks the first enabled `Camera` in the current
scene each frame and hands the result to the backend. With no camera, it falls back to
an orthographic matrix in window pixels, so a scene made entirely of
`RENDERABLE_SCREEN_SPACE` geometry draws correctly without one.

Focal length is interpreted against a 36x24mm frame, so the vertical field of view is
`2*atan(12 / focalLength)`.

### `Renderable` — the abstract base
The base for anything that contributes geometry. Enrolls with the application's backend
in `OnCreate` and withdraws in `OnDestroy`; a subclass overriding `OnCreate` **must**
call `Renderable::OnCreate()`. Its destructor withdraws too, so a bare `delete` is safe.
Enrollment is safe with no backend at all — a headless run simply never draws it.

Subclasses implement `Collect(RenderList &)`, which is called once a frame, after every
`FrameUpdate` and only while `IsEnabled()`. Add nothing to sit the frame out.

A renderable never sees a backend type or a native handle — it emits `DrawCommand`s of
plain vertex data, and the backend consumes them. That is what lets a backend be added
without touching a behavior. Whatever a command points at must stay valid for the whole
render pass, so **emit from storage the renderable owns, never from a local**; every
subclass below keeps its vertices as a member for exactly that reason.

Public surface beyond `Collect`: `GetRenderableFlags()`, `GetLayer()` /
`SetLayer(int32_t)` — lower draws first within a band, ties fall back to enrollment
order — and `IsEnrolled()`.

`RenderableType` is gone, replaced by `RenderableFlags` in the `ApplicationFlags`
idiom: a `uint32_t` bit set with `RENDERABLE_TRANSPARENT`, `RENDERABLE_SCREEN_SPACE`,
`RENDERABLE_OVERLAY` and `RENDERABLE_TEXTURED` masks, and a `RenderableFlag` bit-index
enum beside it. Both live in `gfx.h`, and the header warns that the two are easy to
confuse.

### The concrete renderables

| Type | What it draws |
|---|---|
| `MeshRenderable` | A mesh from the `MeshRegistry`, as indexed triangles. Holds a `Vec4f m_tint`, a `List<DrawVertex>` and a dirty flag |
| `SpriteRenderable` | A textured quad in world space; four vertices, rebuilt when dirty |
| `ShapeRenderable` | A polyline through a list of points. Width above one pixel is expanded into a triangle ribbon **here**, not asked of the backend, because line width is not portable |
| `GeometryRenderable` | The general batched primitive. Public `vertices` / `indices` / `topology`, passed straight through, plus a public `SetRenderableFlags`. Anything the typed renderables miss is expressed as vertices and indices, which every backend already understands |
| `UIRenderable` | Screen-space base. Concrete and registered in its own right — a scene file can name it — but it draws nothing; it owns the rect and the screen-space flags its leaves share |
| `ImageUI` | A textured quad in screen space |
| `TextUI` | Nothing yet. Real text needs a font atlas on SDL_ttf; the type, its ID and its parameters exist so scenes that already name it keep loading. Carries `String m_text` and a `Vec4f m_color` |

### `AudioSource`
Type ID only. Empty private section, no overrides. Registered and inert.

### `Collider`
Type ID only. Not registered, so it cannot currently reach a scene from JSON.
Collision in the pong sample is hand-rolled inside `Ball::FrameUpdate`.

### `UI`
Declares overrides for `FrameUpdate`, `OnStart`, and `OnEnable`, but there is **no
`gui.cpp`** — none of the three are defined, so any target that instantiates a `UI`
fails to link. `gui.h` is also absent from the `ROSE.h` umbrella header, unlike every
other Core behavior. Not registered. Superseded in practice by `UIRenderable` and its
leaves; treat this type as dead.

---

## `examples/game1` — the pong sample

Registered in `examples/game1/main.cpp` under module name `"Game1"`, and referenced
by type ID from `examples/game1/assets/game1scene1.json`.

| Behavior | Type ID | File | Registered | In scene JSON |
|---|---|---|---|---|
| `AppCloser` | `1510c09900c8cc39-21a67c5c20659851` | `applicationcloser.h/.cpp` | ✅ | ✅ "Scene Manager" |
| `Paddle` | `bceacc50f13cee94-7dbb93fec8659973` | `paddle.h/.cpp` | ✅ | ✅ x2 "Paddle1"/"Paddle2" |
| `Ball` | `8dbf834ef011a57f-a9bf2a7d82659778` | `ball.h/.cpp` | ✅ | ✅ "Ball" (+ `Motion`) |
| `FpsCounter` | `995243d320724db5-5fabb4eb31659861` | `fpscounter.h/.cpp` | ✅ | ✅ "FPS Counter" |
| `ResetController` | `ba001a59e555ec31-9b2847c718659683` | `reset.h/.cpp` | ✅ | ✅ |
| `Scoreboard` | `559389588d578314-0479c176496599ed` | `scoreboard.h/.cpp` | ✅ | ❌ not in the scene |

`Puck` is gone — `puck.h` has been deleted, along with the dead type ID
`522b50059cfdc799-09808493ea659876`.

### The `Resetter` mixin

`reset.h` introduces a second, non-`Behavior` base:

```cpp
class Resetter {
  friend class ResetController;
public:
  virtual ~Resetter() {}
protected:
  virtual void Reset() {}
};
```

A behavior that wants to be resettable inherits from **both** `Behavior` and
`Resetter` — `Ball` and `Scoreboard` both do. This is the one place in the tree
where a behavior has a second base, and it is worth knowing because it is what
makes `dynamic_cast` load-bearing: `ResetController::OnStart` sweeps the whole
scene with `Scene::ForEachObject` + `Object::ForEachBehavior`, `dynamic_cast`s
each `Behavior&` to `Resetter*`, and keeps the ones that succeed in a
`List<Resetter *>`. `FrameUpdate` then calls `Reset()` on all of them when `R`
goes down.

That sweep runs in `OnStart`, which is correct — phase two is where reaching
across the scene is legal — and it logs a Debug line per behavior tried, which is
noisy but is also the clearest worked example of the two `ForEach` helpers in the
tree.

### `AppCloser`
Three lines: on every frame, if `InputSystem::GetKey(KeyCode::ESCAPE)`, walk
`GetObject().GetScene().GetApplication()` and call `Quit()`. Doubles as the smallest
worked example of the back-pointer chain.

### `Paddle`
Player-controlled paddle, and the only behavior that exercises the full
Unpack -> Create -> Start -> Update sequence.

- `Unpack` reads one int, `player` (`1` or `2`), into the `Player` enum.
- `OnCreate` picks the key bindings from that value — P1 gets left/right, P2 gets A/D. Self-only work, correctly placed in phase one.
- `OnStart` reaches outward: grabs the SDL window and renderer through the application, caches the window size, and parks itself at `x = screenW/2`, `y` at the bottom edge for P1 or the top for P2 (`margin` 24px).
- `FrameUpdate` moves along x at 500 units/s scaled by `Time::deltaTime`, clamps to the screen, then draws itself as a white `SDL_FRect`.

Exposes `static constexpr float width{120}` / `height{24}`, which `Ball` reads at
compile time for its collision test. Both scene objects share one type ID and differ
only by their `factoryParameters`.

### `Ball`
The moving ball, and a `Resetter`. Owns no motion state itself — the scene JSON
attaches a `Motion` alongside it and `Ball` drives that.

- `OnStart` caches the SDL renderer and window size, calls `Reset()`, then looks up `"Paddle1"` and `"Paddle2"` via `Scene::FindObjectByName` and caches the two `Object*`.
- `Reset()` is the `Resetter` override, so it is driven by `ResetController` rather than by `Ball` watching the keyboard itself. It re-seeds the ball: random x, y at mid-screen, and a random launch angle in `[pi/8, 7pi/8]` with a random up/down sign, written into the sibling `Motion` at speed 300. It finds that sibling with `FindBehavior<Motion>()` and logs an error if it is missing.
- `FrameUpdate` reflects `v.x` off the left/right screen edges, then does an AABB overlap test against both paddles, flipping `v.y` only when the ball is actually heading into the paddle so it cannot stick. Finally draws a 10px white square.

Good reference for the "cache your neighbors in `OnStart`, use them in
`FrameUpdate`" pattern, and for behaviors cooperating on one object.

### `FpsCounter`
Accumulates frame count and elapsed `Time::deltaTime`, recomputing an averaged FPS
every 125 ms, and renders it in a borderless auto-sized half-transparent ImGui
window. `OnStart` is overridden but empty. Purely a HUD overlay — its object's
transform is ignored.

### `Scoreboard`
Registered, but **a stub and not in the scene file**. It derives from `Behavior` and
`Resetter`, holds `m_p1score` / `m_p2score`, and overrides three hooks — of which
only `Reset()` has a body (it zeroes both scores). `OnStart` and `FrameUpdate` are
empty, so nothing scores and nothing is drawn. Wire it into
`game1scene1.json` once it does something.

---

## `examples/orbits` — the n-body demo

Registered in `examples/orbits/main.cpp` under module name `"Orbits"`; scene is
`examples/orbits/assets/orbits.json`. Everything lives in `namespace Orbits`.

| Behavior | Type ID | File | Registered | In scene JSON |
|---|---|---|---|---|
| `PointCloud` | `6592694121c0a7d9-ea7b9a70926599a7` | `pointcloud.h/.cpp` | ✅ | ✅ |
| `Closer` | `f1f08ac7480fbd15-6c1353928d659a6b` | `closer.h` | ✅ | ✅ |

### `PointCloud`
**The reference example for a backend-agnostic `Renderable`**, and the reason to
read this example before writing a new one. It includes no SDL header, resolves no
renderer, and names no backend type — it integrates bodies, projects them to window
space, and hands the geometry up through `Collect`. The same demo therefore runs on
SDL's renderer, the software rasterizer and OpenGL with no source change.

Holds `List<Vec3d>` positions and velocities, a `TrailBuffer` / `TrailRenderer` /
`TrailStyle` trio (`trail.h`, `trailrenderer.h` — plain classes, not behaviors), and
a `List<Point> m_screen` of window-space positions sized once in `Unpack` and
overwritten each frame. `integrate(dt)` advances the simulation and
`updateGeometry()` rebuilds what `Collect` emits.

Note `m_bodyVertices` and `m_centerVertex` are **members, not locals** — a
`DrawCommand` keeps the pointer for the whole render pass, so the storage has to
outlive `Collect`. The header says so explicitly; it is the trap the `Renderable`
contract is built around.

### `Closer`
Header-only, three lines, same shape as game1's `AppCloser`: `ESCAPE` calls
`Quit()` through the back-pointer chain. Two examples now carry their own copy of
this — a candidate for promotion into Core.

---

## `examples/cube` — the mesh path

One file, `examples/cube/main.cpp`, with the behavior defined inline in it; scene is
`examples/cube/assets/cube.json`.

| Behavior | Type ID | Registered | In scene JSON |
|---|---|---|---|
| `RotateFunny` | `71a70e9852f27ed4-e177f66081659b53` | ✅ module `"Cube"` | ✅ |

The scene also names `Camera`, `Motion` and `MeshRenderable` from Core, which makes
it the smallest end-to-end test of the mesh path.

### `RotateFunny`
Finds the sibling `Motion` in `OnStart` and calls `CreateBehavior<Motion>()` if it
is absent, retrying in `FrameUpdate` — so it is also the only worked example of
`CreateBehavior` in the tree, and of the fact that the pointer it returns is not
usable until the next initialize pass. Each frame it composes a small
`Quatd::AxisAngle` delta onto the angular velocity, round-tripping through
`FromEuler` / `ToEuler` because `Motion` stores angular velocity as Euler-ish
axis-times-rate rather than as a quaternion.

`main.cpp` also registers a mesh: `MeshRegistry::Get().RegisterMesh(new Mesh(CUBE_MESH), ...)`.
The registry takes ownership, so it gets a heap copy of its own — handing it the
address of the global `CUBE_MESH` would have it delete a static at exit.

---

## Elsewhere

`examples/ControllerTest` and `examples/KeyboardTest` are single-`main.cpp` input
probes; they define no behaviors and do not build a scene. `examples/game2` is a
single `main.cpp` plus `vnstuff.h` and defines none either. `ROSE_Editor`
(`include/ROSE/Editor/`) defines none, and its target is commented out of
`CMakeLists.txt` pending a redesign.

## Quick reference: type IDs

Sorted for grepping a scene file back to a type.

```
03552ebc7a5d6dea-0aecc877e265992b  ImageUI            (Core)
1510c09900c8cc39-21a67c5c20659851  AppCloser          (game1)
1f6b0c7d94a3e582-b0d47e2c3165a91d  GeometryRenderable (Core)
32c5294ce2e000fd-271098999265987f  SpriteRenderable   (Core)
4e503b30bef7e6ef-a5250bb2d5659727  ShapeRenderable    (Core)
559389588d578314-0479c176496599ed  Scoreboard         (game1, stub)
6592694121c0a7d9-ea7b9a70926599a7  PointCloud         (orbits)
71a70e9852f27ed4-e177f66081659b53  RotateFunny        (cube)
7edb97ba340571cc-d568181d0c659920  Collider           (Core, unregistered)
8448e5b94997ad4d-ccee5b44f06598a2  AudioSource        (Core)
8dbf834ef011a57f-a9bf2a7d82659778  Ball               (game1)
98b16c050e659798-2ba97b3cd1a9dd7c  Camera             (Core)
995243d320724db5-5fabb4eb31659861  FpsCounter         (game1)
9cfaac61886d5e2e-9e75d84332659afc  MeshRenderable     (Core)
9f24e65aa0a93483-992866975b659b59  TextUI             (Core)
ab0a57d02d8e9fde-462c4cdfe26597d3  Motion             (Core)
ba001a59e555ec31-9b2847c718659683  ResetController    (game1)
bceacc50f13cee94-7dbb93fec8659973  Paddle             (game1)
cb497ab81ab3dd7b-85272b1b21659b69  UIRenderable       (Core)
f1f08ac7480fbd15-6c1353928d659a6b  Closer             (orbits)
5be3e81fa226cdc4-c8105a5b51659aa0  UI                 (Core, unregistered, dead)
```
