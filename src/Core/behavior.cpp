/**

    @file      behavior.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      08.04.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/ROSE.h>

namespace ROSE {
  Object &Behavior::GetObject() const noexcept { return *m_object; }
  Scene &Behavior::GetScene() const noexcept { return m_object->GetScene(); }
  void Behavior::OnStart() {}
  void Behavior::FrameUpdate() {}
  void Behavior::Unpack(const ParamView &view) {}
  void Behavior::OnEnable() {}
  void Behavior::OnDisable() {}
  Behavior::~Behavior() {}

  void Behavior::OnDestroy() {}

  /* Both guard on the current state: a redundant Enable()/Disable() is a no-op rather than a
   * second OnEnable/OnDisable, so a behavior can treat the pair as balanced. */
  void Behavior::Enable() {
    if (m_enabled) return;
    m_enabled = true;
    this->OnEnable();
  }

  void Behavior::Disable() {
    if (!m_enabled) return;
    m_enabled = false;
    this->OnDisable();
  }

  bool Behavior::IsEnabled() const noexcept { return m_enabled; }

} // namespace ROSE