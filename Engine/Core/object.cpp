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
  Object::Object() = default;

  Object::Object(const char* _name) : Object(_name, {}, {}) {}
  Object::Object(const char* _name, const Transform &_transform) : Object(_name, _transform, {}) {}
  Object::Object(const char* _name, const Transform &_transform, List<UniquePtr<Behavior>> &&_behaviors) :
    m_name(_name), m_transform(), m_pendingAdd(), m_pendingDestroy() {
    for (auto &b : _behaviors) {
      m_behaviors.insert(UUID::Generate(), Move(b));
    }
  }

  void Object::FrameUpdate() noexcept {
    for (auto& b : m_behaviors) {
      b.second->FrameUpdate();
    }
  }

  Scene& Object::GetScene() const noexcept { return *m_scene; }
  Object& Object::GetParent() const noexcept { return *m_parent; }

  void Object::OnStart() noexcept {
    for (auto const &b : m_behaviors) {
      b.second->OnStart();
    }
  }

  void Object::AddBehavior(UniquePtr<Behavior>&& behavior) noexcept {
    m_pendingAdd.push_back(Move(behavior));
  }
  void Object::DestroyBehavior(const UUID &u) noexcept {
    m_pendingDestroy.push_back(u);
  }


  Behavior *Object::GetBehavior(const UUID &u) noexcept {
    auto it = m_behaviors.find(u);
    if (it == m_behaviors.end()) return nullptr;
    return it->second.get();
  }

}
