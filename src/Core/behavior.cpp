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
  void Behavior::FixedUpdate() {}
  void Behavior::Unpack(const ParamView &view) {}
  void Behavior::OnEnable() {}
  void Behavior::OnDisable() {}
  Behavior::~Behavior() {}

}