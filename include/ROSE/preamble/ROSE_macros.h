/**

    @file      ROSE_macros.h
    @brief     
    @details   ~
    @author    Cool Guy
    @date      4.02.2026
    @copyright © Cool Guy, 2026. All right reserved.

**/
#pragma once

#include <type_traits>

#pragma region preprocessor macros

#define ROSE_STR(x) #x
#define ROSE_XSTR(x) ROSE_STR(x)

#define ROSE_CAT(a,b) a##b
#define ROSE_XCAT(a,b) ROSE_CAT(a,b)

#define ROSE_PRAGMA(x) __pragma(x)

#pragma endregion

#pragma region important stuff
#if !(__cplusplus >= 202002L || _MSVC_LANG >= 202002L)
#error ROSE is designed for C++20 or newer!
#endif 

#if !defined(_MSC_VER) || defined(__clang__)
#error ROSE is designed for MSVC ABI!
#endif

#if !defined(_M_AMD64)
#error ROSE is designed for x86-64 CPU architecture! For AMD64 please launch the ROSE runtime through Proton/FEX.
#endif

#if !defined(_WIN32) && !defined(_WIN64)
#error ROSE is designed for 64-bit Windows. For POSIX systems please launch the ROSE runtime through Proton.
#endif

#pragma endregion



#pragma region logic

namespace ROSE {
    template<typename T>
    constexpr void SWAP(T &a, T &b) { T &c = a; a = b; b = c; }
    template<typename T>
    constexpr const T &MIN(const T &a, const T &b) noexcept requires(std::is_arithmetic_v<T>) { return (b < a ? b : a); }
    template<typename T>
    constexpr const T &MAX(const T &a, const T &b) noexcept requires(std::is_arithmetic_v<T>) { return (a < b ? b : a); }

}

#pragma endregion

#define ROSE_API_EXPORT __declspec(dllexport)
#define ROSE_API_IMPORT __declspec(dllimport)

#define ROSE_API(MODULE) CASE_XCAT(ROSE_, CASE_XCAT(MODULE, _API))

#pragma endregion


