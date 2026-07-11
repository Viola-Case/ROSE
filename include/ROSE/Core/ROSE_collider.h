/**

  @file       ROSE_collider.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       10.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once
#include <ROSE/Core/ROSE_behavior.h>

namespace ROSE {
  class Collider : public Behavior {
  public:
    static constexpr UUID typeID = "7edb97ba340571cc-d568181d0c659920"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }
  };
}