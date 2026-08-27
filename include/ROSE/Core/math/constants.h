/**

  @file      constants.h
  @brief
  @details   ~
  @author    Viola Case
  @date      12.03.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <cstdint>
#include <ROSE/Core/bigint.h>

namespace ROSE::math {
  constexpr double PI = 3.14159265358979323846;        //!< π
  constexpr double E = 2.71828182845904523536;         //!< e
  constexpr double PHI = 1.61803398874989484820;       //!< φ
  constexpr double TAU = 2. * PI;                      //!< τ
  constexpr double SQRT2 = 1.41421356237309504880;     //!< sqrt 2)
  constexpr uint32_t FNVPRIME32 = 0x01000193;          //!< 32-bit FNV prime
  constexpr uint64_t FNVPRIME64 = 0x00000100000001b3;  //!< 64-bit FNV prime
  constexpr uint32_t FNVOFFSET32 = 0x811c9dc5;         //!< 32-bit FNV offset basis
  constexpr uint64_t FNVOFFSET64 = 0xcbf29ce484222325; //!< 64-bit FNV offset basis
  constexpr uint128_t FNVPRIME128 = (static_cast<uint128_t>(0x0000000001000000) << 64) |
                                    static_cast<uint128_t>(0x000000000000013B); //!< 128-bit FNV prime
  constexpr uint128_t FNVOFFSET128 = (static_cast<uint128_t>(0x6C62272E07BB0142) << 64) |
                                     static_cast<uint128_t>(0x62B821756295C58D); //!< 128-bit FNV offset basis

  constexpr double MAXFINITE64 = 1.7976931348623157e308;  //!< largest finite double; anything above is +∞
  constexpr float MAXFINITE32 = 3.4028234663852886e38f;   //!< largest finite float; anything above is +∞
  constexpr double MINNORMAL64 = 2.2250738585072014e-308; //!< smallest positive normal double; below this is subnormal
  constexpr float MINNORMAL32 = 1.1754943508222875e-38f;  //!< smallest positive normal float; below this is subnormal

  constexpr uint64_t SQRTMAGIC64 =
    0x5FE6EB50C7B537A9;                        //!< \f$1/\sqrt{v}\f$ seed for a double (the Quake trick, widened)
  constexpr uint32_t SQRTMAGIC32 = 0x5F3759DF; //!< \f$1/\sqrt{v}\f$ seed for a float (the original Quake constant)

  /*! Subnormals seed the inverse-sqrt refinement badly, so they get scaled into the normal range
   *  first and scaled back after the root. UNSCALE is \f$1/\sqrt{\text{SCALE}}\f$, so the pair cancels. */

  constexpr double SUBNORMALSCALE64 = 0x1p106;   //!< \f$2^{106}\f$
  constexpr double SUBNORMALUNSCALE64 = 0x1p-53; //!< \f$2^{-53}\f$
  constexpr float SUBNORMALSCALE32 = 0x1p50f;    //!< \f$2^{50}\f$
  constexpr float SUBNORMALUNSCALE32 = 0x1p-25f; //!< \f$2^{-25}\f$

#define π PI
#define φ PHI
#define τ TAU

} // namespace ROSE::math
