# Internal notes — ROSE Core

Condensed reference for the hand-rolled foundation layers of the engine, so
the headers don't need re-reading from scratch every session.

| File | Covers |
|---|---|
| [`rtl.md`](rtl.md) | `ROSE/Core/rtl.h` — the ROSE Template Library (containers, string, smart pointers, hash map, 128-bit ints) plus its support headers |
| [`math.md`](math.md) | `ROSE/Core/math.h` — vectors, matrices, complex, quaternions, constants, scalar helpers |
| [`known-issues.md`](known-issues.md) | Verified defects and sharp edges in both layers, with the reproduction for each |
| [`scene-object-behavior.md`](scene-object-behavior.md) | The composition layer — `Scene`/`Object`/`Behavior` lifecycle, `BehaviorFactory`, scene JSON and `ParamView` |
| [`behaviors.md`](behaviors.md) | Every concrete `Behavior` in the tree, its type ID, and what it does |
| [`conventions.md`](conventions.md) | House style — comments, formatting, naming, header layering, language rules |

The RTL and math files were checked against the headers as of
**2026-07-23** (branch `master`, at `8510b49`). Behavioural claims were confirmed
by compiling and running the code, not inferred from reading — see
`known-issues.md` for the specific repros. The two composition-layer files were
written against `master` @ `6870ee3` (**2026-07-24**) by reading the sources; the
sharp edges they list are traced to specific lines but were not each run.

## Ground rules for both layers

- Namespace is `ROSE`; math lives in the nested `ROSE::math`. `math.h` re-exports
  only ten concrete typedefs into `ROSE` (see `math.md`) — everything else needs
  a `math::` qualifier.
- The RTL is a deliberate re-implementation of the STL, not a wrapper. Names
  follow STL casing for container members (`size()`, `push_back()`, `begin()`)
  and ROSE casing (`PascalCase`) for free functions (`Move`, `Forward`, `Swap`,
  `MakeUnique`). `HashMap` keeps lower-case members on purpose so range-`for`
  finds `begin()`/`end()`.
- Members are `m_`-prefixed; parameters in the newer files are `_`-prefixed.
- No exceptions in engine logic. The only `throw`s are in `constexpr`-only paths
  (`parse128`) and in `std::formatter::parse` implementations.
- The build is **C++20** (`CMAKE_CXX_STANDARD 20` in `CMakeLists.txt`), even
  though some comments reference C++23/26 features.
- Clang/GCC only in practice: `bigint.h` `#error`s on anything without
  `__int128`, and `mathfunctions.h` leans on `__builtin_*`.

## Header layering

```
stdlib.h        (the only place <cstring>/<cstdint>/<cmath>/<format>/... are pulled in)
  └── typetraits.h      concepts: Character, StdScalar, Scalar, BehaviorType
        └── utility.h   Move/Forward/Swap/Min/Max, MemCpy/MemCmp, StrLen, ByteSwap, FNV decls
              └── everything else
```

`ROSE.h` includes `platform.h` → `macros.h` → `rtl.h` → … → `math.h`, in that
order. **That order matters** — `math/vector.h` uses `ROSE_ASSERT` without
including `macros.h`, so it only compiles because `ROSE.h` got there first. See
`known-issues.md` #1.

## Core is a DLL

`ROSE_Core` is a **shared** library (`build/*/bin/ROSE_Core.dll` plus an import lib
in `build/*/lib`). The point is single instances: `BehaviorFactory::get()`,
`InputSystem::GetInstance()`, `MeshRegistry::Get()`, `AudioSystem::Get()` and
`Time`'s storage exist exactly once in the process, however many modules link Core.

Anything with an out-of-line definition in `src/Core` is annotated `ROSE_API(Core)`,
which expands to `ROSE_Core_API` and resolves in `macros.h`:

| Defined | Meaning |
|---|---|
| `ROSE_Core_BUILD` | `dllexport` — CMake sets this `PRIVATE` on the `ROSE_Core` target only |
| nothing | `dllimport` — every consumer |
| `ROSE_Core_API` (e.g. `-DROSE_Core_API=`) | neither; for standalone builds, see below |

Header-only templates (`List`, `BasicString`, `TypedHashMap`, `UniquePtr`, `Vec`,
`Mat`, …) are deliberately **not** exported — each module instantiates its own copy,
which duplicates code but not state.

Two consequences worth knowing:

- **The dynamic CRT is now load-bearing.** `UniquePtr`/`MakeUnique` and
  `BasicString::allocate` are header-inline, so an executable can `new` an object
  that Core `delete`s and vice versa. That only works while both modules share one
  heap. `x64-windows-static` is no longer a viable triplet.
- **ImGui is a static lib**, so Core and any executable that calls ImGui directly
  each get their own `GImGui`. Game code drawing into `Application`'s UI must call
  `ROSE::AttachImGui()` (from `ROSE/Core/imgui.h`) once after `Application::Init()`
  — see `examples/game1/main.cpp`. Programs that run their own ImGui context
  (`ROSE_Editor`, `ControllerTest`, `KeyboardTest`) must not call it.

## Building the tree

`cmake --build --preset debug` (or `release`) **fails on `ROSE_Editor`, by design**:

```
include/ROSE/Editor/editor.h(3,2): error: ROSE EDITOR MUST BE BUILT WITH EDITOR CONFIG
```

`CMakeLists.txt` adds `ROSE_Editor` in every configuration, but `editor.h` guards on
`ROSE_EDITOR`, which is only defined under `$<$<CONFIG:Editor>:...>`. The target
therefore exists everywhere and compiles in exactly one place. This is intentional,
not a regression — don't "fix" it by relaxing the guard.

What follows from that:

- The default all-target build is unusable outside the `editor` preset. Name the
  targets you actually want:
  ```sh
  cmake --build --preset debug --target ROSE_Core
  cmake --build --preset debug --target Game1 Game2 Orbits KeyboardTest ControllerTest
  ```
- `ENGINE_BUILD` depends on `ROSE_Editor`, so it dies the same way. It only
  aggregates usefully under the `editor` preset.
- `cmake --build --preset editor` builds everything except `ROSE_AssetMaker` and
  `ROSE_UUID_Generator`, which fail there with
  `lld-link: error: /failifmismatch: mismatch detected for '_ITERATOR_DEBUG_LEVEL'`.
  vcpkg has no `Editor` configuration, so CMake falls back to its **debug** imported
  libs (`_ITERATOR_DEBUG_LEVEL=2`) while the `Editor` config compiles with `NDEBUG`
  and the release CRT (`=0`). Only these two targets notice, because `CLI11.lib` is
  the one real static archive they link — every other dependency is an import lib
  and carries no `/failifmismatch` directive. This predates the DLL conversion.

Ninja reports the failure after the targets that already succeeded, so the
`Linking CXX executable ...` lines printed above the error are real — only
`ROSE_Editor` died.

## Compiling a throwaway test against the RTL

Clang is at `/d/Program Files/LLVM/bin/clang++` and is on `PATH`.

```sh
# headers + RawBuffer is enough for List / String / smart pointers / math
clang++ -std=c++20 -w -DROSE_Core_API= -I include test.cpp src/Core/rtl/buffer.cpp -o t.exe
```

`-DROSE_Core_API=` is what makes this work now that Core is a DLL: without it the
headers declare `RawBuffer` and friends `dllimport` while `buffer.cpp` defines them
locally, and the link fills with `LNK4217 locally defined symbol imported`. (It
still produces a working exe, just a noisy one.) Do **not** use
`-DROSE_Core_BUILD` instead — `dllexport` forces every member of an exported class
to be emitted, which drags in `HashMap` and fails to link.

For anything touching `HashMap` you need two extra pieces, because
`src/Core/rtl/hashmap.cpp` includes all of `<ROSE/ROSE.h>` (which drags in SDL,
ImGui, etc.) and depends on `FNV1A64`, which lives in `utility.cpp` next to
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
`PRIVATE` on `ROSE_Core`, so assertions are compiled out in every example and
tool target.
