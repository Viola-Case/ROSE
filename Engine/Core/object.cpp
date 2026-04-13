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
  Object::Object() {

  }

  void Object::FrameUpdate() noexcept {
    for (auto &b : m_behaviors) {
      b.second->FrameUpdate();
    }
  }

  Scene &Object::GetScene() const noexcept { return *m_scene; }
  Object &Object::GetParent() const noexcept { return *m_parent; }
  TypedHashMap<UUID, UniquePtr<Behavior>> &Object::GetBehaviors() noexcept { return m_behaviors; }
}