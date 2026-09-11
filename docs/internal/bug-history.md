# Bug history — ROSE

Defects that are **fixed**, kept because the diagnosis cost more than the patch and the same shape
can recur. This is the counterpart to [`known-issues.md`](known-issues.md), which tracks what is
still broken; an entry moves here once the fix has landed and been verified.

Each entry names the symptom first, because the symptom is what a reader will have in front of them
when they come looking. Do not delete an entry when it stops feeling relevant — the value is entirely
in being findable from a stale error message years later.

---

## `_ITERATOR_DEBUG_LEVEL` mismatch under the Editor config

**Symptom.** `cmake --build --preset editor` built everything except `ROSE_AssetMaker` and
`ROSE_UUID_Generator`, which failed with:

```
lld-link: error: /failifmismatch: mismatch detected for '_ITERATOR_DEBUG_LEVEL'
```

**Cause.** vcpkg has no `Editor` configuration, so CMake fell back to its **debug** imported libs
(`_ITERATOR_DEBUG_LEVEL=2`) while the `Editor` config compiles with `NDEBUG` and the release CRT
(`=0`). Only those two targets noticed, because `CLI11.lib` was the one real static archive they
linked — every other dependency is an import lib and carries no `/failifmismatch` directive.

**Fixed** by `cmake/ROSEVendor.cmake`:

```cmake
set(CMAKE_MAP_IMPORTED_CONFIG_EDITOR Release "")
```

so `Editor` imports the release artifacts it is ABI-compatible with.

**Still live on one path.** The mapping only applies on the vendored path; the `editor-vcpkg` preset
reproduces the original failure, because the toolchain file governs imported configs there.

**The general shape**, which is the reason this entry exists: any custom configuration name needs an
imported-config mapping, or CMake guesses — and it guesses debug. Adding a fourth configuration
alongside Debug/Release/Editor will hit this again, and the symptom will point at the linker rather
than at the mapping.

> `ROSE_AssetMaker` was deleted with the `.rpkg` work (see [`assets.md`](assets.md)). It is named here
> because it is what the error actually said at the time; the failure was never specific to it.
