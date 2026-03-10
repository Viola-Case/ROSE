/**

  @file      object.cpp
  @brief     
  @details   ~
  @author    Viola Case
  @date      2.03.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/


#include <ROSE/ROSE.h>

namespace ROSE {
  Scene *const Object::GetParentScene() const noexcept { return Parent; }
}