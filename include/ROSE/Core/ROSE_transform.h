/**

    @file      ROSE_transform.h
    @brief     Transform struct representing position, rotation, and scale in 3D space
    @details   Transform is a plain-data struct; it holds no logic and is safe
               to copy. Used by Object to describe placement in the world.
    @author    Viola Case
    @date      24.02.2026
    @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_math.h>

namespace ROSE {
  /**
    @struct  Transform
    @brief   6-DoF position, orientation, and scale descriptor for 3D objects.
    @details All three fields use double precision. The rotation is stored as a
             unit quaternion; ensure it is normalised before passing it to the
             rendering or physics layer.
  **/
  struct Transform {
    Vec3d position; //!< World-space translation
    Quatd rotation; //!< Orientation as a unit quaternion
    Vec3d scale;    //!< Per-axis scale factors (1,1,1 = identity)
  };
}
