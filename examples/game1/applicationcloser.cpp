/**
  
  @file      applicationcloser.cpp
  @brief     
  @details   ~
  @author    Viola Case
  @date      07.04.2026
  @copyright © Viola Case, 2026. All rights reserved.
  
**/

#include "applicationcloser.h"

using namespace ROSE;

void AppCloser::FrameUpdate() {
  if (InputSystem::GetKey(KeyCode::ESCAPE)) {
    //__debugbreak();
    GetObject().GetScene().GetApplication().Quit();
  }
}
