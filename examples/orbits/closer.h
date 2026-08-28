/**

    @file      closer.h
    @brief     Quits the application on ESC.
    @details   ~
    @author    Viola Case
    @date      27.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <ROSE/ROSE.h>

namespace Orbits {
  using namespace ROSE;

  class Closer : public Behavior {
  public:
    static constexpr UUID typeID = "f1f08ac7480fbd15-6c1353928d659a6b"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

  protected:
    void FrameUpdate() override {
      if (InputSystem::GetKey(KeyCode::ESCAPE)) m_object->GetScene().GetApplication().Quit();
    }
  };
} // namespace Orbits
