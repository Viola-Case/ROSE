/**

    @file      ROSE_transform.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      24.02.2026
    @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_preamble.h>
#include <ROSE/Core/ROSE_math.h>

namespace ROSE {
  struct Transform {
    
  /*!
   * Transform struct for 6dof position/rotation in 3D space
   */
    Vec3d position;
    Quatd rotation;
  };
}