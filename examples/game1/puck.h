/**

  @file       puck.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       19.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once

#include <ROSE/ROSE.h>

using namespace ROSE;

class Puck : public Behavior {
public:
  static constexpr UUID typeID = "522b50059cfdc799-09808493ea659876"_uuid;
  static constexpr UUID TypeID() { return typeID; }
  UUID GetTypeID() const noexcept override { return typeID; }
protected:
  void FrameUpdate() override;
};