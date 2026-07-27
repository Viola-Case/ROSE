/**

    @file      game1behavior.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      27.07.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <ROSE/ROSE.h>

using namespace ROSE;

class Resetter {
  friend class ResetController;
public:
  virtual ~Resetter() {}
protected:
  virtual void Reset() {}
};

class ResetController : public Behavior {
public:
  constexpr static UUID typeID = "ba001a59e555ec31-9b2847c718659683"_uuid;
  constexpr static UUID TypeID() { return typeID; }
  UUID GetTypeID() const noexcept override { return typeID; }

protected:
  void OnStart() override;
  void FrameUpdate() override;

  List<Resetter *> m_resetters;
};
