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

    for (const UUID &u : m_pendingDestroy) {
      m_objects.erase(u);
    }

    for (UniquePtr<Object> &o : m_pendingAdd) {
      const auto &u = UUID::Generate();
      o->m_uuid = u;
      m_objects.insert(u, Move(o));
    }
    m_pendingAdd.clear();

    for (auto &o : m_objects) {
      for (UniquePtr<Behavior> &b : o.second->m_pendingAdd) {
        b->m_object = o.second.get();
        o.second->m_behaviors.insert(UUID::Generate(), Move(b));
      }
      o.second->m_pendingAdd.clear();
    }

  }

  void Scene::AddObject(Object &&obj) noexcept {
    UniquePtr<Object> o (MakeUnique<Object>(Move(obj)));
    m_pendingAdd.push_back(Move(o));
  }

  Object *Scene::GetObject(const UUID &uuid) noexcept {
    auto it = m_objects.find(uuid);
    if (it != m_objects.end()) return it->second.get();
    return nullptr;
  }
}
