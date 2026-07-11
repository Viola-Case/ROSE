/**

    @file      transform.h
    @brief
    @details   ~
    @author    Viola Case
    @date      24.02.2026
    @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/math.h>

namespace ROSE {
  /*!
   * Transform struct for 6dof position/rotation in 3D space
   */

  struct Transform {
    Vec3d position;
    Quatd rotation;
    Vec3d scale;
  };
} // namespace ROSE