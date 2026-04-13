/**
  
  @file      applicationcloser.cpp
  @brief     
  @details   ~
  @author    Viola Case
  @date      07.04.2026
  @copyright © Viola Case, 2026. All rights reserved.
  
**/

#include "applicationcloser.h"

void AppCloser::FrameUpdate() {
  if (ROSE::InputSystem::GetKey(ROSE::KeyCode::ESCAPE))
    GetObject().GetScene().GetApplication().Quit();
}
