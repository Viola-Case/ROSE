/**

    @file      reset.cpp
    @brief     
    @details   ~
    @author    Viola Case
    @date      27.07.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include "reset.h"

using namespace ROSE;

void ResetController::OnStart() {
  m_object->GetScene().ForEachObject([&](Object &o) {
    o.ForEachBehavior([&](Behavior &b) {
      Log(LogLevel::Debug, "Attempting dynamic cast to resetter on {}-{} from object {} to list\n", b.GetTypeID().high, b.GetTypeID().low, o.GetName());
      if (Resetter *r = dynamic_cast<Resetter *>(&b)) {
        Log(LogLevel::Debug,"Success.\n");
        m_resetters.push_back(r);
      } else {
        Log(LogLevel::Debug, "Failure.\n");
      }
    });
  });
}

void ResetController::FrameUpdate() {
  if (InputSystem::GetKeyDown(KeyCode::R)) {
    for (auto r : m_resetters) {
      r->Reset();
    }
  }
}
