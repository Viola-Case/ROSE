/**

    @file ROSE_platform.h
    @brief Platform defines
    @author Viola Case
    @date 02.04.2026
    @copyright © Viola Case, 2026. All right reserved.

 **/

#pragma once



#if defined(_WIN32)
#define ROSE_PLATFORM_WINDOWS 1
#define ROSE_PLATFORM         "Windows"
#else
#define ROSE_PLATFORM_WINDOWS 0
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define ROSE_PLATFORM_MACOS 1
#define ROSE_PLATFORM       "MacOS"
#else
#define ROSE_PLATFORM_MACOS 0
#endif

#if defined(__linux__)
#define ROSE_PLATFORM_LINUX 1
#define ROSE_PLATFORM       "Linux"
#else
#define ROSE_PLATFORM_LINUX 0
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define ROSE_PLATFORM_BSD 1
#define ROSE_PLATFORM     "BSD"
#else
#define ROSE_PLATFORM_BSD 0
#endif

#if defined(_XBOX_ONE)
#define ROSE_PLATFORM_XBOX_ONE 1
#define ROSE_PLATFORM          "XBox One"
#else
#define ROSE_PLATFORM_XBOX_ONE 0
#endif

#if defined(_GAMING_XBOX_SCARLETT)
#define ROSE_PLATFORM_XBOX_SERIES_XS 1
#define ROSE_PLATFORM                "Xbox Series X/S"
#else
#define ROSE_PLATFORM_XBOX_SERIES_XS 0
#endif

#if defined(__ORBIS__)
#define ROSE_PLATFORM_PS4 1
#define ROSE_PLATFORM     "PlayStation 4"
#else
#define ROSE_PLATFORM_PS4 0
#endif

#if defined(__PROSPERO__)
#define ROSE_PLATFORM_PS5 1
#define ROSE_PLATFORM     "PlayStation 5"
#else
#define ROSE_PLATFORM_PS5 0
#endif

#if defined(__NX__)
#define ROSE_PLATFORM_SWITCH 1
#define ROSE_PLATFORM        "Nintendo Switch"
#else
#define ROSE_PLATFORM_SWITCH 0
#endif

#if defined(__clang__)
#define ROSE_COMPILER_CLANG 1
#define ROSE_COMPILER_GCC   0
#define ROSE_COMPILER_MSVC  0
#define ROSE_COMPILER       "Clang"
#elif defined(__GNUC__)
#define ROSE_COMPILER_CLANG 0
#define ROSE_COMPILER_GCC   1
#define ROSE_COMPILER_MSVC  0
#define ROSE_COMPILER       "GCC"
#elif defined(_MSC_VER)
#define ROSE_COMPILER_CLANG 0
#define ROSE_COMPILER_GCC   0
#define ROSE_COMPILER_MSVC  1
#define ROSE_COMPILER       "MSVC"
#else
#error "Unknown or unsupported compiler"
#endif

#if defined(__NVCC__)
#define ROSE_COMPILER_CUDA 1
#warning what the fuck why are you compiling this with cuda are you fucking insane
#else
#define ROSE_COMPILER_CUDA 0
#endif

#if ROSE_PLATFORM_XBOX_ONE || ROSE_PLATFORM_XBOX_SERIES_XS
#define ROSE_PLATFORM_XBOX 1
#else
#define ROSE_PLATFORM_XBOX 0
#endif

#if ROSE_PLATFORM_PS4 || ROSE_PLATFORM_PS5
#define ROSE_PLATFORM_PLAYSTATION 1
#else
#define ROSE_PLATFORM_PLAYSTATION 0
#endif

#if ROSE_PLATFORM_PLAYSTATION || ROSE_PLATFORM_XBOX_SERIES_XS || ROSE_PLATFORM_SWITCH
#define ROSE_PLATFORM_CONSOLE 1
#else
#define ROSE_PLATFORM_CONSOLE 0
#endif

#if ROSE_PLATFORM_MACOS || ROSE_PLATFORM_LINUX || ROSE_PLATFORM_BSD
#define ROSE_KERNEL_POSIX 1
#else
#define ROSE_KERNEL_POSIX 0
#endif

#if ROSE_PLATFORM_WINDOWS || ROSE_PLATFORM_MACOS || ROSE_PLATFORM_LINUX || ROSE_PLATFORM_BSD
#define ROSE_PLATFORM_DESKTOP 1
#else
#define ROSE_PLATFORM_DESKTOP 0
#endif

#if !defined(__x86_64__) && !defined(_M_X64)
#error "ROSE requires AMD64/x86-64"
#endif
// TODO i mean... does it really though

#if !(__cplusplus >= 202002L || _MSVC_LANG >= 202002L)
#error "ROSE is designed for C++20 or newer!"
#endif
// TODO make a fallback in case someone at like CERN or somewhere has some super legacy bullshit

