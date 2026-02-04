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

#pragma region versioning

constexpr unsigned char  ROSE_VERSION_MAJOR = 0;
constexpr unsigned char  ROSE_VERSION_MINOR = 0;
constexpr unsigned short ROSE_VERSION_PATCH = 1;

constexpr unsigned int   ROSE_VERSIONNUM(char major, char minor, short patch) { return (1000000 * major + 10000 * minor + patch); }

constexpr unsigned char  ROSE_VERSIONNUM_MAJOR(unsigned int version) { return (version / 1000000); }
constexpr unsigned char  ROSE_VERSIONNUM_MINOR(unsigned int version) { return ((version % 1000000) / 10000); }
constexpr unsigned short ROSE_VERSIONNUM_PATCH(unsigned int version) { return (version % 10000); }

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


#pragma endregion


