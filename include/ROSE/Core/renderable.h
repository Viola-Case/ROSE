/**

  @file       renderable.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       10.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once

#include <ROSE/Core/behavior.h>

namespace ROSE {
  enum class RenderableType {
    Sprite,
    Mesh,
    UI,
    InstancedMesh,
    
  };

  class Renderable : public Behavior {
  public:
    static constexpr UUID typeID = "0f01169dcb6855ae-daead65ffc659aec"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }
  private:

  };
}