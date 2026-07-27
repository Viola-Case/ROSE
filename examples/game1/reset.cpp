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
  auto l = m_object->GetScene().GetObjects();
  for (auto &o : l) {
    for (auto &b : o->GetBehaviors()) {
      Log(LogLevel::Info, "Attempting dynamic cast to resetter on {}-{} from object {} to list\n", b->GetTypeID().high, b->GetTypeID().low, o->GetName());
      if (Resetter *r = dynamic_cast<Resetter *>(b)) {
        Log(LogLevel::Info,"Success.\n");
        m_resetters.push_back(r);
      } else {
        Log(LogLevel::Info, "Failure.\n");
      }
    }
  }
}

void ResetController::FrameUpdate() {
  if (InputSystem::GetKeyDown(KeyCode::R)) {
    for (auto r : m_resetters) {
      r->Reset();
    }
  }
}
