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

/* TODO dead file - there is no puck.cpp, so FrameUpdate is never defined, the type is
 * never registered in main.cpp, and nothing references it. Superseded by Ball. Delete it. */
class Puck : public Behavior {
public:
  static constexpr UUID typeID = "522b50059cfdc799-09808493ea659876"_uuid;
  static constexpr UUID TypeID() { return typeID; }
  UUID GetTypeID() const noexcept override { return typeID; }
protected:
  void FrameUpdate() override;
};