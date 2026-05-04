/**

  @file      ROSE_macros.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      4.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <type_traits>

#pragma region preprocessor macros

#define ROSE_STR(x) #x
#define ROSE_XSTR(x) ROSE_STR(x)

#define ROSE_CAT(a,b) a##b
#define ROSE_XCAT(a,b) ROSE_CAT(a,b)

#define ROSE_PRAGMA(x) __pragma(x)

#ifdef _DEBUG

#include <cstdio>
#include <cstdlib>

#if defined(_MSC_VER)
#define ROSE_DEBUG_BREAK() __debugbreak()
#else
#define ROSE_DEBUG_BREAK() __builtin_trap()
#endif

#define ROSE_ASSERT(expr)                                                   \
    do {                                                                      \
      if (!(expr)) {                                                          \
        std::fprintf(stderr,                                                  \
          "Assertion failed!\n  Expr: %s\n  File: %s\n  Line: %d\n",          \
          #expr, __FILE__, __LINE__);                                         \
        ROSE_DEBUG_BREAK();                                                   \
        std::abort();                                                         \
      }                                                                       \
    } while (0)

#define ROSE_ASSERT_MSG(expr, msg)                                            \
  do {                                                                        \
    if (!(expr)) {                                                            \
      std::fprintf(stderr,                                                    \
        "Assertion failed!\n  Expr: %s\n  Msg: %s\n  File: %s\n  Line: %d\n", \
        #expr, msg, __FILE__, __LINE__);                                      \
      ROSE_DEBUG_BREAK();                                                     \
      std::abort();                                                           \
    }                                                                         \
  } while (0)

#else

#define ROSE_ASSERT(expr) ((void)0)
#define ROSE_ASSERT_MSG(expr, msg) ((void)0)

#endif

#define ROSE_ENUM_CONSTRUCTOR(TYPE, NAME, ...) \
  enum Value : TYPE { __VA_ARGS__ } value; \
  constexpr NAME() = default; \
  constexpr NAME(Value v) : value(v) {}                                      \
  constexpr NAME(TYPE v) : value(static_cast<Value>(v)) {}                   \
                                                                             \
  constexpr operator Value() const { return value; }                         \
  constexpr explicit operator TYPE() const { return static_cast<TYPE>(value); } \

#pragma endregion

#pragma region logic

namespace ROSE {


}

#pragma endregion

#pragma region API bullshit

#define ROSE_API_EXPORT __declspec(dllexport)
#define ROSE_API_IMPORT __declspec(dllimport)

#define ROSE_API(MODULE) CASE_XCAT(ROSE_, CASE_XCAT(MODULE, _API))

#pragma endregion
