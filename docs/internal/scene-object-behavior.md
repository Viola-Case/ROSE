# Scene / Object / Behavior — internal reference

The runtime composition layer: `Application` owns `Scene`s, a `Scene` owns
`Object`s, an `Object` owns `Behavior`s, and gameplay code lives in behaviors.

Checked against the headers and sources on **2026-07-24** (`master` @ `6870ee3`).
Where the header comment and the code disagree, this file describes **the code**;
those spots are flagged.

| Piece | Header | Source |
|---|---|---|
| `Scene` | `include/ROSE/Core/scene.h` | `src/Core/scene.cpp` |
| `Object` | `include/ROSE/Core/object.h` | `src/Core/object.cpp` |
| `Behavior` | `include/ROSE/Core/behavior.h` | `src/Core/behavior.cpp` |
| `BehaviorFactory` | `include/ROSE/Core/factory.h` | `src/Core/factory.cpp` |
| `ParamView` | `include/ROSE/Core/paramview.h` | `src/Core/paramview.cpp` |

For the list of concrete behavior types, see [`behaviors.md`](behaviors.md).

---

## 1. Ownership

```
Application
  List<Scene> m_scenes            (by value; m_currentScene is a Scene* into the list)
  └── Scene
        TypedHashMap<UUID, UniquePtr<Object>> m_objects
        List<UniquePtr<Object>>               m_pendingAdd
        List<UUID>                            m_pendingDestroy
        Application*                          m_application     ← set by Bind()
        └── Object
              TypedHashMap<UUID, UniquePtr<Behavior>> m_behaviors   ← keyed by GetTypeID()
              List<UniquePtr<Behavior>>               m_pendingAdd
              List<UUID>                              m_pendingDestroy
              Scene*                                  m_scene
              Object*                                 m_parent      ← never assigned anywhere
              Transform                               transform     ← public
              └── Behavior
                    Object* m_object   ← set when the behavior is installed
```

Back-pointers give you the whole chain from inside a behavior:

```cpp
GetObject()                                   // Object&
GetObject().GetScene()                        // Scene&
GetObject().GetScene().GetApplication()       // Application&
```

`m_behaviors` is keyed by the behavior's **type ID**, not by an instance UUID.
That is the design, and it means **one instance per behavior type per object** —
`CreateBehavior<T>()` on an object that already has a `T` hands back the existing
one. Two paddles means two `Object`s, not two `Paddle`s on one object.

`Scene` is move-only (`Scene(Scene&&)`/`operator=(Scene&&)` defaulted, no copy)
and stores raw back-pointers, so **`Bind()` must be re-called after any move** —
`Application::Init()` does this for every scene once the `List<Scene>` has settled
at its final address.

Scenes reach an application through `ApplicationInitSettings`, which owns them
until `Init` moves the whole `List<Scene>` across (see
[`application.md`](application.md) §2). `m_currentScene` points at the **first**
scene added, or is `nullptr` when the list is empty — an application with no
scenes is legal and every scene call in the frame loop is null-guarded.

## 2. Lifecycle

Three phases, in order, per the contract in `behavior.h`:

| Phase | Hook | Rule |
|---|---|---|
| Create | `OnCreate()` | Touch only yourself. No reaching into the scene — the neighbors may not exist yet. |
| Start | `OnStart()` | Everyone in the batch has finished `OnCreate()`. Now you may look up other objects and cache pointers. |
| Update | `FrameUpdate()` | Once per frame, forever. |

The ordering guarantee is *batch-wide*: `Scene::OnStart()` and
`Scene::InitializePendingBehaviors()` both run `OnCreate()` over **every** behavior
in the batch before running `OnStart()` on any of them. Within a phase the order
is hash-map iteration order — unspecified, do not depend on it.

### Where each hook is driven from

```
Application::Run()                                       src/Core/application.cpp
  loop:
    snapshot previous keyboard state
    if (m_currentScene && scene changed since last frame) m_currentScene->OnStart()
    SDL_PollEvent
    Time::dT = <frame duration>
    m_currentScene->FrameUpdate()
      ├── for each object: Object::FrameUpdate()
      │     └── for each behavior: Behavior::FrameUpdate()
      ├── erase scene-level m_pendingDestroy
      ├── erase each object's behavior m_pendingDestroy
      ├── promote scene m_pendingAdd  (assigns a fresh UUID::Generate(), sets m_scene)
      └── InitializePendingBehaviors()
    sleep out the rest of the frame budget
```

Both scene calls are guarded on `m_currentScene` being non-null. The full loop,
including render-backend and ImGui ordering and the frame pacing, is in
[`application.md`](application.md) §5.

`Scene::OnStart()` fires once per scene *activation*, guarded by a
`static Scene *lastScene` in `Application::Run()`. It walks every behavior with
`OnCreate()`, then every behavior with `OnStart()`, then drains pending behaviors.

`Scene::InitializePendingBehaviors()` is the steady-state path. It loops until the
scene settles, so a behavior spawned inside another behavior's `OnCreate()`/
`OnStart()` still gets its own Create → Start before the next `FrameUpdate()`:

```cpp
for (;;) {
  // move every object's m_pendingAdd into m_behaviors, set m_object, collect ptrs
  if (newBehaviors.empty()) break;
  for (Behavior *b : newBehaviors) b->OnCreate();   // phase one
  for (Behavior *b : newBehaviors) b->OnStart();    // phase two
}
```

### Hooks that exist but are never called

`FixedUpdate()`, `OnEnable()`, `OnDisable()`, and `UnpackParameters()` are declared,
have empty base implementations, and have **no call sites anywhere in the engine**.
`m_enabled` is likewise set to `true` and never read. `FixedUpdate`'s own doc
comment admits it ("Currently this doesn't get called"). Overriding any of them is
dead code today — `UI` (`gui.h`) overrides `OnEnable()` and nothing calls it.

Note also `Behavior::Unpack()` is the deserialization hook the loader actually
calls; `UnpackParameters()` is a vestigial second spelling. Override `Unpack`.

## 3. Scene

```cpp
class Scene final {
public:
  Application &GetApplication() const noexcept;

  void    AddObject(Object &&) noexcept;            // queued; lands next FrameUpdate
  Object *FindObjectByName(const StringView &) noexcept;   // linear scan, nullptr if absent
  Object *GetObject(const UUID &) noexcept;                // hash lookup, nullptr if absent
  void    DestroyObject(const UUID &) noexcept;     // queued; erased next FrameUpdate

  static Scene FromJSONString(const String &) noexcept;
  Scene(Scene &&) noexcept = default;
  Scene &operator=(Scene &&) noexcept = default;

private:
  Scene();                                          // auto-names itself "Scene<N>"
  void OnStart() noexcept;
  void FrameUpdate() noexcept;
  void InitializePendingBehaviors() noexcept;
  void Bind(Application &) noexcept;
  String ToJSONString() noexcept;
};
```

Adds and destroys are **both deferred to the end of `FrameUpdate()`**, so iterating
the scene while a behavior spawns or kills objects is safe. Ordering inside one
frame: destroy objects → destroy behaviors → promote new objects → initialize new
behaviors.

`AddObject` takes the object by rvalue and re-homes it into a `UniquePtr`. The UUID
you may have set is ignored — the promotion step overwrites it with
`UUID::Generate()`.

There is no public way to construct a `Scene` other than `FromJSONString`; the
default constructor is private and `Application` is a friend.
`ApplicationInitSettings::AddSceneFromFile(path)` is the convenience wrapper —
it reads the file whole and calls `FromJSONString` for you, logging and skipping
a file that will not open.

`ToJSONString()` is a stub: it emits `{"name": ...}` and drops the objects on the
floor. Round-tripping a scene does not work yet.

## 4. Object

```cpp
class Object final {
public:
  Object();
  explicit Object(const char *name);
  Object(const char *name, const Transform &);
  Object(const char *name, const Transform &, List<UniquePtr<Behavior>> &&);

  template <class B> requires std::is_base_of_v<Behavior, B>
  Behavior *CreateBehavior();        // returns existing instance if the type is present

  template <class B> requires std::is_base_of_v<Behavior, B>
  B *FindBehavior() noexcept;        // nullptr if absent

  Scene &GetScene() const noexcept;

  Transform transform;               // public, { position, rotation, scale }

private:
  void OnStart() noexcept;
  void FrameUpdate() noexcept;
  void AddBehavior(UniquePtr<Behavior> &&);
  template <BehaviorType B> void DestroyBehavior();
  Object *GetParent() const noexcept;
  Behavior *GetBehavior(const UUID &) noexcept;
};
```

`FindBehavior<T>()` is the workhorse — behaviors use it in `OnStart()` to cache
siblings (`Ball` grabs its `Motion` this way).

`AddBehavior`, `DestroyBehavior`, and `GetBehavior` are **private**, reachable only
through the `friend class Behavior` / `friend class Scene` declarations. They also
have no callers in the tree right now: today the only paths that install a behavior
are the JSON loader and `CreateBehavior<T>()`.

`transform` is a plain public member with no dirty tracking — write to it directly.
`Transform` is `{ Vec3d position; Quatd rotation; Vec3d scale; }`; note the
default-constructed `Object` leaves `scale` at `{0,0,0}` (the in-class initializer
covers only position and rotation).

There are no public accessors for an object's name or UUID.

## 5. Behavior

```cpp
class Behavior {
  friend class Object;
  friend class Scene;
protected:
  virtual void OnCreate() {}
  virtual void OnStart();
  virtual void FrameUpdate();
  virtual void FixedUpdate();                       // never called
  virtual void Unpack(const ParamView &);           // deserialization hook
  virtual void OnEnable();                          // never called
  virtual void OnDisable();                         // never called
public:
  virtual UUID GetTypeID() const noexcept = 0;      // the one pure virtual
  virtual ~Behavior();
  virtual void UnpackParameters(const ParamView &); // vestigial, never called
  Object &GetObject() const noexcept;
protected:
  UUID    m_uuid;                                   // never assigned
  Object *m_object { nullptr };
  bool    m_enabled { true };                       // never read
};
```

All the lifecycle hooks are `protected` — the engine reaches them through
friendship. Overrides in your subclass should stay `protected` too (every existing
behavior does).

`m_object` is valid from `OnCreate()` onward; the loader and
`InitializePendingBehaviors()` both set it before any hook runs. Inside a behavior
`m_object` and `GetObject()` are interchangeable, and the codebase uses both.

`Behaviour` is a `using` alias for `Behavior`, if you prefer the other spelling.

### Declaring one

```cpp
class MyBehavior : public Behavior {
public:
  static constexpr UUID typeID = "ba12c4ae50659b9a-91cc2a6057b9e054"_uuid;
  static constexpr UUID TypeID() noexcept { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }
protected:
  void Unpack(const ParamView &view) override;
  void OnCreate() override;
  void OnStart() override;
  void FrameUpdate() override;
};
```

Generate a fresh UUID for every new behavior type — `src/Tools/UUIDGenerator`
builds a CLI that prints one in the right format. Two behaviors sharing a type ID
collide in the factory (second registration is rejected with a warning) and in
`Object::m_behaviors` (they cannot coexist on one object).

Two concepts describe behavior types:

| Concept | Where | Requires |
|---|---|---|
| `BehaviorType<T>` | `typetraits.h` | `std::derived_from<T, Behavior>` |
| `RegistrableBehavior<T>` | `behavior.h` | derived, default-initializable, **and** `T::TypeID()` returning `UUID` |

`MakeBehavior<T>` takes the second, which is what makes the "must be default
constructible" rule real: the factory mints a blank instance and `Unpack()` fills it
in. A behavior with only a non-trivial constructor cannot be registered. `Paddle`
shows the pattern — a defaulted-in-spirit `Paddle() noexcept` that installs sane
member values, with the JSON overriding them in `Unpack()`.

## 6. BehaviorFactory

```cpp
using FactoryFn = UniquePtr<Behavior> (*)();

template <RegistrableBehavior T> UniquePtr<Behavior> MakeBehavior();

class BehaviorFactory {
public:
  static BehaviorFactory &get();                                    // singleton
  static RegisterResult Register(FactoryFn, const UUID &id, const char *moduleName = "");
  static void RegisterModule(const char *moduleName);
  static UniquePtr<Behavior> Create(const UUID &id) noexcept;       // nullptr if unregistered
};
```

Core lives in `ROSE_Core.dll`, so there is exactly one `BehaviorFactory` in the
process no matter how many modules register into it. `FactoryFn` is a raw function
pointer, so a game's `MakeBehavior<T>` instantiated in the executable registers and
runs fine across that boundary.

`RegisterResult` is `Success | DuplicateID | Failure`. A duplicate ID logs a warning
and keeps the **first** registration.

Registration is manual and must happen **before any scene is loaded** — the loader
resolves type IDs through the factory, and an unregistered type is unrecoverable at
that point. Scenes are now parsed while the `ApplicationInitSettings` are being
built (`AddSceneFromFile`), which is earlier than it used to be: register
everything before you touch the settings object, not merely before `Init`. Core
registers its own behaviors via
`extern "C" void RoseRegisterCoreModule(BehaviorFactory&)`; a game does the same for
its types:

```cpp
BehaviorFactory &factory = BehaviorFactory::get();
RoseRegisterCoreModule(factory);
Pair<FactoryFn, UUID> fns[] {
  { MakeBehavior<AppCloser>,  AppCloser::TypeID()  },
  { MakeBehavior<Paddle>,     Paddle::TypeID()     },
  { MakeBehavior<FpsCounter>, FpsCounter::TypeID() },
  { MakeBehavior<Ball>,       Ball::TypeID()       },
};
for (const auto &p : fns) factory.Register(p.first, p.second, "Game1");
```

Each module records its name with `factory.RegisterModule("Game1")` once the
`Register` calls are done. This used to be a `reinterpret_cast` of the factory to
its first member; with Core in a DLL, `BehaviorFactory`'s layout is part of the ABI
and a member reorder would have corrupted memory across the boundary with no
diagnostic, so the cast is gone.

## 7. Scene JSON and `ParamView`

`Scene::FromJSONString` parses (nlohmann) this shape:

```json
{
  "name": "scene1",
  "objects": [
    {
      "name": "Paddle1",
      "uuid": "aaaaaaaaaaaaaaaa-bbbbbbbbbbbbbbbc",
      "transform": {
        "position": [0, 0, 0],
        "rotation": [0, 0, 0],
        "scale":    [1, 1, 1]
      },
      "behaviors": [
        {
          "typeid": "bceacc50f13cee94-7dbb93fec8659973",
          "factoryParameters": { "player": 1 }
        }
      ]
    }
  ]
}
```

- UUID strings are `16 hex - 16 hex`; a wrong-length string yields `UUID::Invalid()`.
- `rotation` is **Euler angles**, converted via `Quatd::FromEuler`.
- Per behavior: look up `typeid` in the factory → mint → set `m_object` → `Unpack(factoryParameters)` → insert keyed by type ID. Lifecycle hooks run later, from `Scene::OnStart()`.
- All four keys (`name`, `uuid`, `transform`, `behaviors`) and the three transform keys are read with `.at()`, so a missing one throws, is caught, logged as `"Scene string corrupt"`, and returns a **partially populated** scene.
- Behavior parameters are the only forgiving part — see below.

`ParamView` is the read-only accessor handed to `Unpack()`. It wraps a `const void*`
JSON node so nlohmann stays out of the public headers.

```cpp
int     GetInt   (const String &key, int    fallback = 0)  const noexcept;
double  GetDouble(const String &key, double fallback = 0)  const noexcept;
bool    GetBool  (const String &key, bool   fallback = 0)  const noexcept;
String  GetString(const String &key, const String &fallback = "") const noexcept;
Vec3d   GetVec3d (const String &key, Vec3d fallback = {0}) const noexcept;
UUID    GetUUID  (const String &key) const noexcept;        // UUID::Invalid() on failure
ParamView Child  (const String &key) const noexcept;        // nested objects; null view if absent
```

Every getter is `noexcept` and falls back rather than failing: a null node, a
missing key, and a wrong-typed key all produce the fallback. The header calls this
out as a known gap — missing (honest version skew) and wrong-typed (possible
corruption) deserve different treatment and currently get none. **Survive
everything, detect nothing.**

A typical `Unpack`:

```cpp
void Motion::Unpack(const ParamView &view) {
  m_drdt = view.GetVec3d("drdt", {});
  m_dTdt = view.GetVec3d("dTdt", {});
}
```

## 8. Sharp edges

Each verified against the source at `6870ee3`.

1. **`Object::CreateBehavior<T>()` skips the whole lifecycle.** It inserts straight
   into `m_behaviors` rather than `m_pendingAdd`, so the scene's initializer never
   sees it: `m_object` stays `nullptr` and `OnCreate()`/`OnStart()` never run. The
   first `FrameUpdate()` then calls into a behavior whose `GetObject()` dereferences
   null. It also does not check the factory result, so an unregistered `T` inserts a
   null `UniquePtr` and the next frame crashes. Use it only for types with no
   lifecycle hooks, or fix it to route through `AddBehavior`.

2. **The JSON loader dereferences a null factory result.** `FromJSONString` does
   `bvr->m_object = &obj` immediately after `BehaviorFactory::Create(tID)`. An
   unregistered or misspelled `typeid` in the scene file is a null-pointer crash
   during load, not the "log loudly, skip, limp forward" the design comment
   describes.

3. **`Object(name, transform, behaviors)` calls `std::terminate()`.** The loop body
   in `object.cpp:22-25` inserts and then unconditionally terminates, so the
   4-argument constructor is unusable with a non-empty list. The same constructor
   also ignores its `Transform` argument (it initializes `transform()` instead of
   `transform(_transform)`), which means the 3-argument overload silently discards
   the transform too.

4. **`ParamView::GetDouble` rejects integers.** It gates on
   `is_number_float()`, so `"focalLength": 30` falls through to the fallback while
   `30.0` works. `Camera::Unpack` reads both `focalLength` and `orthographic` through
   `GetDouble`, so a JSON `true`/`false` or a whole number for either is silently
   ignored. `GetInt` uses the wider `is_number()` and does not have this problem.

5. **Objects loaded from JSON have a default `m_uuid`.** The parsed UUID becomes the
   *map key* but is never written to `obj.m_uuid`. Only objects promoted through
   `AddObject` get their member set (to a freshly generated value, not yours). Any
   future code reading `Object::m_uuid` will disagree with `Scene::GetObject`.

6. **`Behavior::m_uuid` is never assigned at all** — it exists, it is default
   constructed, nothing writes it.

7. **`Object::m_parent` is never assigned.** `GetParent()` always returns `nullptr`;
   there is no scene graph hierarchy yet, only a flat object list.

8. **`Scene::ToJSONString()` does not serialize objects** (§3). It also builds an
   unused local struct per object — the loop body has no effect.

9. **Behaviors erased from `m_pendingDestroy` get no shutdown callback.** The map
   entry is erased and the destructor runs; there is no `OnDestroy`.

10. **Scene switching re-runs `OnStart()` on the whole scene.** The guard in
    `Application::Run()` is a `static Scene*`, so returning to a previously active
    scene replays `OnCreate()` **and** `OnStart()` over behaviors that already ran
    them. There is only one scene in practice today, so this has not bitten yet.
