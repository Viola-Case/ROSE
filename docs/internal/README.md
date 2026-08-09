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
- `cmake --build --preset editor` builds the whole tree, editor included.

Ninja reports the failure after the targets that already succeeded, so the
`Linking CXX executable ...` lines printed above the error are real — only
`ROSE_Editor` died.

## Compiling a throwaway test against the RTL

Clang is at `/d/Program Files/LLVM/bin/clang++` and is on `PATH`.

```sh
# headers + RawBuffer is enough for List / String / smart pointers / math
clang++ -std=c++20 -w -I include test.cpp src/Core/rtl/buffer.cpp -o t.exe
```

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
