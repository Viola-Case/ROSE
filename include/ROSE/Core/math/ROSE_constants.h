/**

  @file      ROSE_constants.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      12.03.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <cstdint>
#include <ROSE/Core/math/ROSE_complex.h>

namespace ROSE::math {
  constexpr float PI =  3.141592653589793f;             //!< π
  constexpr float E =   2.718281828459045f;             //!< e
  constexpr float PHI = 1.618033988749895f;             //!< φτ
  constexpr float TAU = 2.f * PI;                       //!< τ
  constexpr float SQRT2 = 1.41421356237309504;          //!< sqrt 2)
  constexpr Compd I{ 0,1 };                             //!< i (sqrt -1)
  constexpr uint32_t FNVPRIME32 = 0x01000193;           //!< 32-bit FNV prime
  constexpr uint64_t FNVPRIME64 = 0x00000100000001b3;   //!< 64-bit FNV prime
  constexpr uint32_t FNVOFFSET32 = 0x811c9dc5;          //!< 32-bit FNV prime
  constexpr uint64_t FNVOFFSET64 = 0xcbf29ce484222325;  //!< 64-bit FNV prime

}
