#line 2 "examples/orbits/simcontrol.h"
/**

  @file       simcontrol.h
  @brief      Keyboard control of the simulation, and the escape hatch out of it.
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/ROSE.h>

using namespace ROSE;

class PointCloud;
class Tracer;

/*!
 * Hotkeys for everything the HUD exposes as a widget, so the simulation can be driven without taking a hand off
 * the camera keys. Camera hotkeys live in `OrbitCamera` itself; this one owns the simulation and the application.
 */
class SimControl : public Behavior {
public:
  static constexpr UUID typeID = "e462fd3697152f62-febad57f6965988b"_uuid;
  static constexpr UUID TypeID() noexcept { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }

protected:
  void OnStart() override;
  void FrameUpdate() override;

  PointCloud *m_cloud { nullptr };
  Tracer *m_tracer { nullptr };
};
