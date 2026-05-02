/**
  
  @file      applicationcloser.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      07.04.2026
  @copyright © Viola Case, 2026. All rights reserved.
  
**/

#pragma once
#include "ROSE/ROSE.h"

using namespace ROSE;

class AppCloser : public Behavior {
protected:
  void FrameUpdate() override;
};
