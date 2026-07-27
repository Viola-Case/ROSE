/**

    @file      scoreboard.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      27.07.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <ROSE/ROSE.h>

#include "reset.h"

using namespace ROSE;
class Scoreboard : public Behavior, public Resetter {
public:
  constexpr static UUID typeID = "559389588d578314-0479c176496599ed"_uuid;
  constexpr static UUID TypeID() noexcept { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }
protected:
  void OnStart() override;
  void FrameUpdate() override;
  void Reset() override;

  int m_p1score{};
  int m_p2score{};
};
