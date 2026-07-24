# Behavior catalogue

Every type deriving from `ROSE::Behavior` in the tree as of **2026-07-24**
(`master` @ `6870ee3`), with its type ID and what it actually does. For the
lifecycle, factory, and JSON contract these all plug into, see
[`scene-object-behavior.md`](scene-object-behavior.md).

"Registered" means the type is wired into a `BehaviorFactory` at startup and can
therefore appear in a scene JSON file. Unregistered types can only be installed in
C++, and today nothing does that either.

---

## Core (`ROSE_Core`)

Registered by `RoseRegisterCoreModule` in `src/Core/factory.cpp`.

| Behavior | Type ID | Header | Registered | State |
|---|---|---|---|---|
| `Camera` | `98b16c050e659798-2ba97b3cd1a9dd7c` | `Core/camera.h` | ✅ | Data only |
| `Motion` | `ab0a57d02d8e9fde-462c4cdfe26597d3` | `Core/motion.h` | ✅ | **Implemented** |
| `Renderable` | `0f01169dcb6855ae-daead65ffc659aec` | `Core/renderable.h` | ✅ | Empty stub |
| `AudioSource` | `8448e5b94997ad4d-ccee5b44f06598a2` | `Core/audiosource.h` | ✅ | Empty stub |
| `Collider` | `7edb97ba340571cc-d568181d0c659920` | `Core/collider.h` | ❌ | Empty stub |
| `UI` | `5be3e81fa226cdc4-c8105a5b51659aa0` | `Core/gui.h` | ❌ | Declared, **unimplemented** |

### `Motion`
The only Core behavior with a working `FrameUpdate`. Integrates the owning object's
`Transform` from four state vectors — velocity `m_drdt`, angular velocity `m_dTdt`,
acceleration `m_d2rdt2`, angular acceleration `m_d2Tdt2` — using **semi-implicit
Euler**: rates advance first, then position and rotation integrate against the
end-of-frame velocities, which keeps it from pumping energy in when acceleration is
nonzero. Angular velocity is axis-times-rate: direction is the axis, magnitude the
rate in rad/s, applied as a left-composed `Quatd::AxisAngle` delta (world space, to
match position) and re-normalized every frame so accumulated rounding does not show
up as the object slowly scaling.

Public setters/getters for all four vectors; the getters return non-const references,
which is how `Ball` flips velocity components in place on a bounce.

`Unpack` reads `drdt` and `dTdt` as `Vec3d`. The accelerations are not deserializable
— set them from code.

### `Camera`
Carries `m_aspectRatio` (`Vec2<int16_t>`), `m_focalLength` (millimeters, default 30),
and `m_orthographic`. `Unpack` reads `focalLength` and `orthographic`, both through
`ParamView::GetDouble`, which only accepts JSON floats — an integer `30` or a boolean
`true` falls back to the default (see sharp edge #4 in the API doc). Aspect ratio is
declared but not unpacked ("do something with the aspect ratio"). No update hook, and
nothing in the renderer consumes a `Camera` yet.

### `Renderable`
Type ID and the `RenderableType` enum (`Sprite`, `Mesh`, `UI`, `InstancedMesh`,
`Rect`) and nothing else — no members, no overrides. Registered, so it can be
attached from JSON, where it is inert.

### `AudioSource`
Type ID only. Empty private section, no overrides. Registered and inert.

### `Collider`
Type ID only. Not registered, so it cannot currently reach a scene from JSON.
Collision in the sample game is hand-rolled inside `Ball::FrameUpdate`.

### `UI`
Declares overrides for `FrameUpdate`, `OnStart`, and `OnEnable`, but there is **no
`gui.cpp`** — none of the three are defined, so any target that instantiates a `UI`
fails to link. `gui.h` is also absent from the `ROSE.h` umbrella header, unlike every
other Core behavior. Not registered. Note `OnEnable` is a dead hook engine-wide
(nothing calls it). Treat this type as a placeholder.

---

## `examples/game1` — the pong sample

Registered in `examples/game1/main.cpp` under module name `"Game1"`, and referenced
by type ID from `examples/game1/assets/game1scene1.json`.

| Behavior | Type ID | File | Registered | In scene JSON |
|---|---|---|---|---|
| `AppCloser` | `1510c09900c8cc39-21a67c5c20659851` | `applicationcloser.h/.cpp` | ✅ | ✅ "Scene Manager" |
| `Paddle` | `bceacc50f13cee94-7dbb93fec8659973` | `paddle.h/.cpp` | ✅ | ✅ ×2 "Paddle1"/"Paddle2" |
| `Ball` | `8dbf834ef011a57f-a9bf2a7d82659778` | `ball.h/.cpp` | ✅ | ✅ "Ball" (+ `Motion`) |
| `FpsCounter` | `995243d320724db5-5fabb4eb31659861` | `fpscounter.h/.cpp` | ✅ | ✅ "FPS Counter" |
| `Puck` | `522b50059cfdc799-09808493ea659876` | `puck.h` | ❌ | ❌ |

### `AppCloser`
Three lines: on every frame, if `InputSystem::GetKey(KeyCode::ESCAPE)`, walk
`GetObject().GetScene().GetApplication()` and call `Quit()`. Doubles as the smallest
worked example of the back-pointer chain.

### `Paddle`
Player-controlled paddle, and the only behavior that exercises the full
Unpack → Create → Start → Update sequence.

- `Unpack` reads one int, `player` (`1` or `2`), into the `Player` enum.
- `OnCreate` picks the key bindings from that value — P1 gets ←/→, P2 gets A/D. Self-only work, correctly placed in phase one.
- `OnStart` reaches outward: grabs the SDL window and renderer through the application, caches the window size, and parks itself at `x = screenW/2`, `y` at the bottom edge for P1 or the top for P2 (`margin` 24px).
- `FrameUpdate` moves along x at 500 units/s scaled by `Time::deltaTime`, clamps to the screen, then draws itself as a white `SDL_FRect`.

Exposes `static constexpr float width{120}` / `height{24}`, which `Ball` reads at
compile time for its collision test. Both scene objects share one type ID and differ
only by their `factoryParameters`.

### `Ball`
The moving ball. Owns no motion state itself — the scene JSON attaches a `Motion`
alongside it and `Ball` drives that.

- `OnStart` caches the SDL renderer and window size, calls `Reset()`, then looks up `"Paddle1"` and `"Paddle2"` via `Scene::FindObjectByName` and caches the two `Object*`.
- `Reset()` re-seeds the ball: random x, y at mid-screen, and a random launch angle in `[π/8, 7π/8]` with a random up/down sign, written into the sibling `Motion` at speed 300. It finds that sibling with `FindBehavior<Motion>()` and logs an error if it is missing.
- `FrameUpdate` re-resets on `R`, reflects `v.x` off the left/right screen edges, then does an AABB overlap test against both paddles, flipping `v.y` only when the ball is actually heading into the paddle so it cannot stick. Finally draws a 10px white square.

Good reference for the "cache your neighbors in `OnStart`, use them in
`FrameUpdate`" pattern, and for behaviors cooperating on one object.

### `FpsCounter`
Accumulates frame count and elapsed `Time::deltaTime`, recomputing an averaged FPS
every 125 ms, and renders it in a borderless auto-sized half-transparent ImGui
window. `OnStart` is overridden but empty. Purely a HUD overlay — its object's
transform is ignored.

### `Puck`
Declares a type ID and a `FrameUpdate` override, but there is **no `puck.cpp`** —
`FrameUpdate` is never defined, the type is not registered, and nothing references
it. Superseded by `Ball`. Dead file.

---

## Elsewhere

`examples/ControllerTest` and `examples/KeyboardTest` are single-`main.cpp` input
probes; they define no behaviors and do not build a scene. `ROSE_Editor`
(`include/ROSE/Editor/`) defines none either.

## Quick reference: type IDs

Sorted for grepping a scene file back to a type.

```
0f01169dcb6855ae-daead65ffc659aec  Renderable   (Core)
1510c09900c8cc39-21a67c5c20659851  AppCloser    (game1)
522b50059cfdc799-09808493ea659876  Puck         (game1, dead)
5be3e81fa226cdc4-c8105a5b51659aa0  UI           (Core, unimplemented)
7edb97ba340571cc-d568181d0c659920  Collider     (Core)
8448e5b94997ad4d-ccee5b44f06598a2  AudioSource  (Core)
8dbf834ef011a57f-a9bf2a7d82659778  Ball         (game1)
98b16c050e659798-2ba97b3cd1a9dd7c  Camera       (Core)
995243d320724db5-5fabb4eb31659861  FpsCounter   (game1)
ab0a57d02d8e9fde-462c4cdfe26597d3  Motion       (Core)
bceacc50f13cee94-7dbb93fec8659973  Paddle       (game1)
```
