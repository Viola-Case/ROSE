#line 2 "examples/orbits/simhud.h"
/**

  @file       simhud.h
  @brief      ImGui panel: conserved quantities, integrator controls, and the drift history plot.
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
class OrbitCamera;

/*!
 * The instrument panel. Reads `PointCloud::GetStats()` and drives the same setters the keyboard does, so the
 * sliders and the hotkeys stay in agreement without either owning the state.
 *
 * The number worth watching is the relative energy drift. For a conservative field the total energy of the cloud
 * is a constant of the motion, so whatever the plot shows is error introduced by the integrator and nothing else -
 * which makes it a direct, quantitative comparison between the four methods on identical initial conditions.
 */
class SimHud : public Behavior {
public:
  static constexpr UUID typeID = "0f0e523c6a83116f-9c02cc20ca65992a"_uuid;
  static constexpr UUID TypeID() noexcept { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }

protected:
  void OnStart() override;
  void FrameUpdate() override;

  static constexpr size_t historyLength { 320 };
  static constexpr double historyInterval { 0.1 };   //!< wall seconds between plot samples

  float m_cloudDrift[historyLength] {};
  float m_tracerDrift[historyLength] {};
  size_t m_historyHead { 0 };
  double m_historyTimer { 0. };

  double m_fps { 0. };
  double m_fpsAccum { 0. };
  int m_fpsFrames { 0 };

  PointCloud *m_cloud { nullptr };
  Tracer *m_tracer { nullptr };
  OrbitCamera *m_camera { nullptr };
};
