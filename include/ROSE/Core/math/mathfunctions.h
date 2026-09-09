/**

  @file      mathfunctions.h
  @brief
  @details   ~
  @author    Viola Case
  @date      11.03.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

/*!
 * Umbrella for the scalar math functions. Each group lives in its own header under `math/functions/` and can be
 * included on its own; this pulls in all of them, and is what `math.h` and the `Vec`/`Mat`/`Quat` headers use.
 *
 * - `functions/common.h`      - `Abs`, `Min`, `Max`, `Clamp`, `IsNaN`, `IsInf`, `IsFinite`
 * - `functions/roots.h`       - `Sqrt`, `Hypot`
 * - `functions/trig.h`        - `Sin`, `Cos`, `Tan`, `Asin`, `Acos`, `Atan`, `Atan2`
 * - `functions/exponential.h` - `Exp`, `Ln`, `Log2`, `Log10`, `Pow`
 *
 * Everything is usable in a constant expression. At runtime each function lowers to the hardware instruction or
 * libm call; at compile time it goes through a hand-written path in the same header. All but `Sin`, `Cos` and `Tan`
 * fold to the correctly rounded result and so agree with the runtime call wherever libm is itself correctly
 * rounded; those three are within an ulp on both paths but may differ in the last bit between them.
 *
 * TODO migrate to C+
 */
#include <ROSE/Core/math/functions/common.h>
#include <ROSE/Core/math/functions/roots.h>
#include <ROSE/Core/math/functions/trig.h>
#include <ROSE/Core/math/functions/exponential.h>
