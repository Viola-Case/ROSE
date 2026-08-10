/**

  @file      macros.h
  @brief
  @details   ~
  @author    Viola Case
  @date      4.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <type_traits>
#include <ROSE/Core/platform.h>

#pragma region preprocessor macros

#if ROSE_COMPILER_CLANG || ROSE_COMPILER_GCC
  #define ROSE_RESTRICT __restrict__
#elif ROSE_COMPILER_MSVC
  #define ROSE_RESTRICT __restrict
#else
  #define ROSE_RESTRICT
#endif

#if ROSE_COMPILER_MSVC

  #define ROSE_FORCEINLINE     __forceinline
  #define ROSE_DEPRECATED(msg) __declspec(deprecated(msg))
  #define ROSE_NOINLINE        __declspec(noinline)
  #define ROSE_ALIGN(x)        __declspec(align(x))
  #define ROSE_NOVTABLE        __declspec(novtable)

#else

  #define ROSE_FORCEINLINE     __attribute__((always_inline)) inline
  #define ROSE_DEPRECATED(msg) __attribute__((deprecated(msg)))
  #define ROSE_ALIGN(x)        __attribute__((aligned(x)))
  #define ROSE_NOVTABLE

#endif

#if ROSE_COMPILER_CLANG || ROSE_COMPILER_GCC
  #define ROSE_LIKELY(x)   (x) [[likely]]
  #define ROSE_UNLIKELY(x) (x) [[unlikely]]
  #define ROSE_PURE        __attribute__((pure))
  #define ROSE_CONST       __attribute__((const))
#else
  #define ROSE_LIKELY(x)   (x)
  #define ROSE_UNLIKELY(x) (x)
  #define ROSE_PURE
  #define ROSE_CONST
#endif

#if ROSE_COMPILER_MSVC || ROSE_COMPILER_CLANG
  #define ROSE_CDECL __cdecl
#else
  #define ROSE_CDECL __attribute__((cdecl))
#endif


#pragma region API bullshit

#if ROSE_PLATFORM_WINDOWS
  #define ROSE_API_EXPORT __declspec(dllexport)
  #define ROSE_API_IMPORT __declspec(dllimport)
  #define ROSE_STDCALL    __stdcall
#else
  #define ROSE_API_EXPORT __attribute__((visibility("default")))
  #define ROSE_API_IMPORT
  #define ROSE_STDCALL
#endif

// Expands to ROSE_<MODULE>_API, e.g. ROSE_API(Core) -> ROSE_Core_API. Each module
// defines its own switch below off a build-time define that CMake sets PRIVATE on
// the module's target, so the module's own TUs export and everybody else imports.
#define ROSE_API(MODULE) ROSE_XCAT(ROSE_, ROSE_XCAT(MODULE, _API))

// The guard is an escape hatch for standalone builds that compile Core sources
// straight into an executable (see docs/internal/README.md): -DROSE_Core_API=
// neutralizes the annotation instead of importing from a DLL that isn't there.
#if !defined(ROSE_Core_API)
  #if defined(ROSE_Core_BUILD)
    #define ROSE_Core_API ROSE_API_EXPORT
  #else
    #define ROSE_Core_API ROSE_API_IMPORT
  #endif
#endif

#pragma endregion

#define ROSE_STR(x)  #x
#define ROSE_XSTR(x) ROSE_STR(x)

#define ROSE_CAT(a, b)  a##b
#define ROSE_XCAT(a, b) ROSE_CAT(a, b)

#define ROSE_PRAGMA(x) __pragma(x)




#ifdef _DEBUG

  #include <cstdio>
  #include <cstdlib>

  #if defined(_MSC_VER)
    #define ROSE_DEBUG_BREAK() __debugbreak()
  #else
    #define ROSE_DEBUG_BREAK() __builtin_trap()
  #endif

  #define ROSE_ASSERT(expr)                                                     \
    do {                                                                        \
      if (!(expr)) {                                                            \
        std::fprintf(stderr,                                                    \
                     "Assertion failed!\n  Expr: %s\n  File: %s\n  Line: %d\n", \
                     #expr, __FILE__, __LINE__);                                \
        ROSE_DEBUG_BREAK();                                                     \
        std::abort();                                                           \
      }                                                                         \
    } while (0)

  #define ROSE_ASSERT_MSG(expr, msg)                                                       \
    do {                                                                                   \
      if (!(expr)) {                                                                       \
        std::fprintf(stderr,                                                               \
                     "Assertion failed!\n  Expr: %s\n  Msg: %s\n  File: %s\n  Line: %d\n", \
                     #expr, msg, __FILE__, __LINE__);                                      \
        ROSE_DEBUG_BREAK();                                                                \
        std::abort();                                                                      \
      }                                                                                    \
    } while (0)

#else

  #define ROSE_ASSERT(expr)          ((void)0)
  #define ROSE_ASSERT_MSG(expr, msg) ((void)0)

#endif

#define ROSE_ENUM_CONSTRUCTOR(TYPE, NAME, ...)             \
  enum Value : TYPE { __VA_ARGS__ } value;                 \
  constexpr NAME() = default;                              \
  constexpr NAME(Value v) : value(v) {}                    \
  constexpr NAME(TYPE v) : value(static_cast<Value>(v)) {} \
                                                           \
  constexpr operator Value() const { return value; }       \
  constexpr explicit operator TYPE() const { return static_cast<TYPE>(value); }

#pragma endregion

#pragma region logic

namespace ROSE {


}

#pragma endregion
