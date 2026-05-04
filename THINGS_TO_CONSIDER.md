# Things to Consider

## Per-tier SIMD DLLs via LoadLibrary / GetProcAddress

Ship separate DLLs compiled per SIMD tier (`engine_avx2.dll`, `engine_avx.dll`, `engine_sse4.dll`).
At startup, `DetectSimd()` picks the right one, `LoadLibrary` loads it, and `GetProcAddress` fills the dispatch table.

**Advantages over the current function-pointer approach:**
- Each DLL can be compiled with `/arch:AVX2` (MSVC) or `-mavx2` (GCC/Clang), so the compiler
  auto-vectorizes freely throughout the entire module — not just hand-written intrinsic functions.
- Tier DLLs can be updated or hot-swapped without relinking the engine.
- Code pages for unsupported tiers are never loaded into RAM.

**Tradeoffs:**
- Deployment complexity: multiple DLLs to ship and manage.
- `LoadLibrary` failure paths need explicit handling.
- `GetProcAddress` is slow, but only at the resolution step — results are stored in function pointers
  just like the current approach, so call-site cost is identical.

**Worth knowing: Windows delay-load hook**
Delay-load DLLs expose `__pfnDliNotifyHook2`, which fires before the loader resolves a DLL.
This lets you redirect a delay-loaded import to a tier-specific DLL transparently —
callers use normal call syntax with no explicit pointer management.

**Related: GNU IFUNC (Linux/GCC only)**
The dynamic linker resolves a function symbol to the correct implementation once at load time,
giving a direct call with zero runtime overhead. Only relevant for the Linux build.

---

## SIMD dispatch — current state

`Engine/Core/simd.cpp` uses a static function pointer (`g_VecAddI32`) resolved once at startup
via `PickVecAddI32()`. Overhead is one predicted indirect branch per call — negligible in practice.
