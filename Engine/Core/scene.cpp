/**
  
  @file      scene.h.cpp
  @brief     
  @details   ~
  @author    Viola Case
  @date      08.04.2026
  @copyright © Viola Case, 2026. All rights reserved.
  
**/


#include <ROSE/ROSE.h>

namespace ROSE {
  Application &Scene::GetApplication() const noexcept {return *m_application;}

  void Scene::FrameUpdate() noexcept {
    for (auto &o : m_objects) {
      o.second->FrameUpdate();
    }
  }
}
