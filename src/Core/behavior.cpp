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
  void Behavior::OnStart() {}
  void Behavior::FrameUpdate() {}
  void Behavior::Unpack(const ParamView &view) {}
  void Behavior::OnEnable() {}
  void Behavior::OnDisable() {}
  Behavior::~Behavior() {}

  void Behavior::OnDestroy() {}

  void Behavior::Enable() {
    m_enabled = true;
    this->OnEnable();
  }

  void Behavior::Disable() {
    m_enabled = false;
    this->OnDisable();
  }

  bool Behavior::IsEnabled() const noexcept { return m_enabled; }

} // namespace ROSE