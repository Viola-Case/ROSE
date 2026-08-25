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
  /* TODO _transform is dropped on the floor - the member init is `transform()`, not
   * `transform(_transform)`, so both this and the 2-arg overload silently discard it.
   *
   * TODO the loop body terminates unconditionally, so passing a non-empty behavior list
   * kills the process. Also the insert keys by UUID::Generate() instead of b->GetTypeID(),
   * which contradicts how every other path keys m_behaviors. Finish or delete this ctor. */
  Object::Object(
    const char* _name,
    const Transform &_transform,
    List<UniquePtr<Behavior>> &&_behaviors
    ) :
  m_name(_name),
  transform(_transform),
  m_pendingAdd(),
  m_pendingDestroy() {
    for (auto &b : _behaviors) {
      m_behaviors.insert(b->GetTypeID(), Move(b));
      // std::terminate();
    }
  }

  void Object::FrameUpdate() noexcept {
    for (auto& b : m_behaviors) {
      Behavior *bb = b.second.get();
      if (bb->m_enabled)
        bb->FrameUpdate();
    }
  }

  Scene& Object::GetScene() const noexcept { return *m_scene; }
  Object *Object::GetParent() const noexcept { return m_parent; }

  void Object::OnStart() noexcept {
    for (auto const &b : m_behaviors) {
      b.second->OnStart();
    }
  }

  void Object::AddBehavior(UniquePtr<Behavior>&& behavior) {
    m_pendingAdd.push_back(Move(behavior));
  }

  Behavior *Object::GetBehavior(const UUID &u) noexcept {
    auto it = m_behaviors.find(u);
    if (it == m_behaviors.end()) return nullptr;
    return it->second.get();
  }

  const char *Object::GetName() const noexcept { return m_name.c_str(); }

}
