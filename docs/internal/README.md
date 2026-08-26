# Internal notes — ROSE Core

Condensed reference for the hand-rolled foundation layers of the engine, so the headers don't need re-reading from
scratch every session.

| File                                                   | Covers                                                                                                                              |
|--------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------|
| [`rtl.md`](rtl.md)                                     | `ROSE/Core/rtl.h` — the ROSE Template Library (containers, string, smart pointers, hash map, 128-bit ints) plus its support headers |
| [`math.md`](math.md)                                   | `ROSE/Core/math.h` — vectors, matrices, complex, quaternions, constants, scalar helpers                                             |
| [`known-issues.md`](known-issues.md)                   | Verified defects and sharp edges in both layers, with the reproduction for each                                                     |
| [`application.md`](application.md)                     | `Application` and `ApplicationInitSettings` — startup, flags, the frame loop, renderer selection                                    |
| [`scene-object-behavior.md`](scene-object-behavior.md) | The composition layer — `Scene`/`Object`/`Behavior` lifecycle, `BehaviorFactory`, scene JSON and `ParamView`                        |
| [`behaviors.md`](behaviors.md)                         | Every concrete `Behavior` in the tree, its type ID, and what it does                                                                |
| [`conventions.md`](conventions.md)                     | House style — comments, formatting, naming, header layering, language rules                                                         |

The RTL and math files were checked against the headers as of **2026-07-23** (branch `master`, at `8510b49`).
Behavioural claims were confirmed by compiling and running the code, not inferred from reading — see
`known-issues.md` for the specific repros. The two composition-layer files were written against `master` @ `6870ee3`
(**2026-07-24**) by reading the sources; the sharp edges they list are traced to specific lines but were not each run.
`application.md` was written against `master` @ `9e2683c` plus the working-tree
`ApplicationInitSettings` change (**2026-08-16**), and its startup claims were confirmed by building and running
`Game1`.

## Ground rules for both layers

- Namespace is `ROSE`; math lives in the nested `ROSE::math`. `math.h` re-exports only ten concrete typedefs into `ROSE`
  (see `math.md`) — everything else needs a `math::` qualifier.
- The RTL is a deliberate re-implementation of the STL, not a wrapper. Names follow STL casing for container members
  (`size()`, `push_back()`, `begin()`)
  and ROSE casing (`PascalCase`) for free functions (`Move`, `Forward`, `Swap`,
  `MakeUnique`). `HashMap` keeps lower-case members on purpose so range-`for`
  finds `begin()`/`end()`.
- Members are `m_`-prefixed; parameters in the newer files are `_`-prefixed.
- No exceptions in engine logic. The only `throw`s are in `constexpr`-only paths (`parse128`) and in
  `std::formatter::parse` implementations.
- The build is **C++20** (`CMAKE_CXX_STANDARD 20` in `CMakeLists.txt`), even though some comments reference C++23/26
  features.
- Clang/GCC only in practice: `bigint.h` `#error`s on anything without
  `__int128`, and `mathfunctions.h` leans on `__builtin_*`.

## Header layering

```
stdlib.h        (the only place <cstring>/<cstdint>/<cmath>/<format>/... are pulled in)
  └── typetraits.h      concepts: Character, StdScalar, Scalar, BehaviorType
        └── utility.h   Move/Forward/Swap/Min/Max, MemCpy/MemCmp, StrLen, ByteSwap, FNV decls
              └── everything else
```

`ROSE.h` includes `platform.h` → `macros.h` → `api.h` → `rtl.h` → … → `math.h`, in that order. **That order matters** —
`math/vector.h` uses `ROSE_ASSERT` without including `macros.h`, so it only compiles because `ROSE.h` got there first.
See
`known-issues.md` #1.

## Core is a DLL

`ROSE_Core` is a **shared** library (`build/*/bin/ROSE_Core.dll` plus an import lib in `build/*/lib`). The point is
single instances: `BehaviorFactory::get()`,
`InputSystem::GetInstance()`, `MeshRegistry::Get()`, `AudioSystem::Get()` and
`Time`'s storage exist exactly once in the process, however many modules link Core.

Anything with an out-of-line definition in `src/Core` is annotated `ROSE_API(CORE)`, which expands to `ROSE_CORE_API`
and resolves in `api.h`:

| Defined                                   | Meaning                                                                |
|-------------------------------------------|------------------------------------------------------------------------|
| `ROSE_Core_BUILD`                         | `dllexport` — CMake sets this `PRIVATE` on the `ROSE_Core` target only |
| nothing                                   | `dllimport` — every consumer                                           |
| `ROSE_CORE_API` (e.g. `-DROSE_CORE_API=`) | neither; for standalone builds, see below                              |

Header-only templates (`List`, `BasicString`, `TypedHashMap`, `UniquePtr`, `Vec`,
`Mat`, …) are deliberately **not** exported — each module instantiates its own copy, which duplicates code but not
state.

The same reasoning applies to non-template value types that carry no global state.
`Transform` used to be exported; it is now header-inline, because the boundary was wrapping a single vector add in an
import thunk to protect state it does not have. Its layout was already part of the ABI — `Object` embeds one by value —
so consumers recompiled on any change regardless, and inlining additionally lets its methods be
`constexpr`, which an exported out-of-line definition can never be. Use the same test for anything new: **export it if
it owns process-wide state, inline it if it is just data with math attached.**

Two consequences worth knowing:

- **The dynamic CRT is now load-bearing.** `UniquePtr`/`MakeUnique` and
  `BasicString::allocate` are header-inline, so an executable can `new` an object that Core `delete`s and vice versa.
  That only works while both modules share one heap. `x64-windows-static` is no longer a viable triplet.
- **ImGui is a static lib**, so Core and any executable that calls ImGui directly each get their own `GImGui`. Game code
  drawing into `Application`'s UI must call
  `ROSE::AttachImGui()` (from `ROSE/Core/imgui.h`) once after `Application::Init()`
  — see `examples/game1/main.cpp`. Programs that run their own ImGui context (`ROSE_Editor`, `ControllerTest`,
  `KeyboardTest`) must not call it.

## Where dependencies come from

`.vendor/install`, built from the tags pinned in `dependencies.toml` by
`./vendor.ps1` (or `./vendor.sh`). The root `CMakeLists.txt` puts that prefix on
`CMAKE_PREFIX_PATH` via `cmake/ROSEVendor.cmake` and every `find_package` resolves there. vcpkg remains available behind
`-DROSE_USE_VCPKG=ON` and the `*-vcpkg`
presets.

Three things about that tree are load-bearing rather than incidental:

- **Release and Debug install into one prefix.** `install(EXPORT)` writes one
  `…Targets-<config>.cmake` per configuration, so installing twice yields a single set of imported targets carrying
  both. Debug artifacts get a `d`
  suffix (`CMAKE_DEBUG_POSTFIX`) so they can coexist.
- **Shared libraries, dynamic CRT, always.** `cmake/vendor.cmake` forces
  `BUILD_SHARED_LIBS=ON` and `CMAKE_MSVC_RUNTIME_LIBRARY=…DLL` (with
  `CMP0091=NEW`, without which the latter is silently ignored) for the same reason the `x64-windows-static` triplet was
  ruled out: `ROSE_Core` is a DLL and the engine allocates across that boundary, so every module must share one heap.
- **imgui is the deliberate exception and is STATIC**, built by the shim in
  `cmake/vendor/imgui/`. Each linking module gets its own `GImGui`, which is what
  `ROSE::AttachImGui()` exists to rebind.

Dependency DLLs are copied next to each binary by `rose_deploy_dlls()`, which uses `$<TARGET_RUNTIME_DLLS:…>` — the
replacement for vcpkg's applocal deployment. `rose_deploy_runtime()` calls it alongside `rose_deploy_crt()`.

## Building the tree

Every preset builds its full target list; `cmake --build --preset debug` (or
`release`, or `editor`) needs no `--target` argument.

```sh
cmake --build --preset debug
```

**`ROSE_Editor` is currently commented out**, pending a redesign. Three blocks in
`CMakeLists.txt` go together and must be restored together — they are marked with CMake bracket comments (`#[[ … ]]`):

1. the `ROSE_BUILD_EDITOR` configuration check, just below `CMAKE_CONFIGURATION_TYPES`
2. the `ROSE_Editor` target block, at the top of the Tools section
3. the `add_dependencies(ENGINE_BUILD ROSE_Editor)` edge at the bottom

`ENGINE_BUILD` therefore aggregates `ROSE_Core` alone right now.

The `Editor` configuration itself is untouched and still works: `ROSE_EDITOR` is defined under `$<$<CONFIG:Editor>:...>`
for `ROSE_Core`, and `editor.h` still refuses to compile without it —

```
include/ROSE/Editor/editor.h(3,2): error: ROSE EDITOR MUST BE BUILT WITH EDITOR CONFIG
```

— so whatever the redesign lands on, editor translation units still cannot leak into a non-Editor build. Don't relax
that guard.

When the target comes back, two things about the check are worth keeping:

- Compare with `STREQUAL "Editor"`, not `MATCHES`. `MATCHES` is a case-sensitive regex doing a substring match, which is
  both too loose and easy to get subtly wrong.
- It reads `CMAKE_BUILD_TYPE`, which only single-config generators set. Every preset here is single-config Ninja. Under
  a multi-config generator (Visual Studio, Ninja Multi-Config) the target is dropped, and the commented-out block warns
  at configure time rather than letting it vanish silently — CMake cannot express "this target exists in one
  configuration only" for multi-config generators.

### Historical: `_ITERATOR_DEBUG_LEVEL` under the Editor config

`cmake --build --preset editor` used to build everything except `ROSE_AssetMaker`
and `ROSE_UUID_Generator`, which failed with
`lld-link: error: /failifmismatch: mismatch detected for '_ITERATOR_DEBUG_LEVEL'`. vcpkg has no `Editor` configuration,
so CMake fell back to its **debug** imported libs (`_ITERATOR_DEBUG_LEVEL=2`) while the `Editor` config compiles with
`NDEBUG`
and the release CRT (`=0`). Only these two targets noticed, because `CLI11.lib` was the one real static archive they
link — every other dependency is an import lib and carries no `/failifmismatch` directive.

**Fixed** by `cmake/ROSEVendor.cmake`, which sets

```cmake
set(CMAKE_MAP_IMPORTED_CONFIG_EDITOR Release "")
```

so `Editor` imports the release artifacts it is ABI-compatible with. The mapping only applies on the vendored path; the
`editor-vcpkg` preset still has the original failure, since the toolchain file governs imported configs there.

## Compiling a throwaway test against the RTL

Clang is at `/d/Program Files/LLVM/bin/clang++` and is on `PATH`.

```sh
# headers + RawBuffer is enough for List / String / smart pointers / math
clang++ -std=c++20 -w -DROSE_CORE_API= -I include test.cpp src/Core/rtl/buffer.cpp -o t.exe
```

`-DROSE_CORE_API=` is what makes this work now that Core is a DLL: without it the headers declare `RawBuffer` and
friends `dllimport` while `buffer.cpp` defines them locally, and the link fills with
`LNK4217 locally defined symbol imported`. (It still produces a working exe, just a noisy one.) Do **not** use
`-DROSE_Core_BUILD` instead — `dllexport` forces every member of an exported class to be emitted, which drags in
`HashMap` and fails to link.

For anything touching `HashMap` you need two extra pieces, because
`src/Core/rtl/hashmap.cpp` includes all of `<ROSE/ROSE.h>` (which drags in SDL, ImGui, etc.) and depends on `FNV1A64`,
which lives in `utility.cpp` next to
`UUID::Generate` and its `<intrin.h>` dependency:

```sh
# 1. strip the umbrella include down to what hashmap.cpp actually needs
sed 's|#include <ROSE/ROSE.h>|#include <ROSE/Core/macros.h>\n#include <ROSE/Core/rtl.h>|' \
    src/Core/rtl/hashmap.cpp > /tmp/hashmap_local.cpp
# 2. supply a 12-line FNV1A64 shim (offset basis + prime are in math/constants.h)
clang++ -std=c++20 -w -I include test.cpp /tmp/hashmap_local.cpp fnv_shim.cpp \
        src/Core/rtl/buffer.cpp -o t.exe
```

Use `-w` or expect noise: `List::operator==` is non-`const`, which trips
`-Wambiguous-reversed-operator` at every call site under C++20.

Define `_DEBUG` to get live `ROSE_ASSERT`s. Note the real build only defines it
`PRIVATE` on `ROSE_Core`, so assertions are compiled out in every example and tool target.
