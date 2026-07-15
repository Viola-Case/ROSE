/**

  @file       mesh.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       14.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once

#include <ROSE/Core/math.h>

namespace ROSE {
  /*!
   *
   */
  struct Vert {
    Vec3d position;
    Vec3d normal;
    Vec2d texCoord;
  };

  /*!
   *
   */
  struct Mesh {
    List<Vert> vertices;
    List<uint32_t> indices;
  };
  /*!
   *
   */
  struct MeshInstance {
    //todo
  };
}
