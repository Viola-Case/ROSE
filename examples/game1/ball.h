/**

  @file       ball.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       22.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once

#include <ROSE/ROSE.h>

using namespace ROSE;

class Ball : public Behavior {
public:
  static constexpr UUID typeID = "8dbf834ef011a57f-a9bf2a7d82659778"_uuid;
  static constexpr UUID TypeID() noexcept { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }
protected:
  void OnStart() override;
  void FrameUpdate() override;
  void Reset();
  void *m_renderer{nullptr};
  Motion *m_motion{nullptr};
  Object *m_paddles[2]{nullptr, nullptr};
};
