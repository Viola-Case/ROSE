/**
  
  @file      applicationcloser.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      07.04.2026
  @copyright © Viola Case, 2026. All rights reserved.
  
**/

#pragma once
#include <ROSE/ROSE.h>

using namespace ROSE;

class AppCloser : public Behavior {
public:
  static constexpr UUID typeID = "1510c09900c8cc39-21a67c5c20659851"_uuid;
  static constexpr UUID TypeID() { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }
protected:
  void FrameUpdate() override;
};
