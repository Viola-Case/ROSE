#line 2 "examples/orbits/simcontrol.cpp"
/**

  @file       simcontrol.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "simcontrol.h"
#include "pointcloud.h"
#include "simcommon.h"
#include "tracer.h"

using namespace ROSE;

namespace {
  constexpr double timeScaleStep { 0.25 };
} // namespace

void SimControl::OnStart() {
  Scene &scene = m_object->GetScene();
  m_cloud = orb::FindInScene<PointCloud>(scene);
  m_tracer = orb::FindInScene<Tracer>(scene);
}

void SimControl::FrameUpdate() {
  if (InputSystem::GetKey(KeyCode::ESCAPE)) {
    m_object->GetScene().GetApplication().Quit();
    return;
  }

  if (!m_cloud) return;

  if (InputSystem::GetKeyDown(KeyCode::SPACE)) m_cloud->TogglePaused();

  /* R replays the same seed, so switching integrator and pressing R compares them on identical initial
   * conditions. N advances the seed when a different cloud is what you actually want. */
  if (InputSystem::GetKeyDown(KeyCode::R)) {
    m_cloud->Reseed();
    if (m_tracer) m_tracer->Reset();
  }
  if (InputSystem::GetKeyDown(KeyCode::N)) {
    m_cloud->SetSeed(m_cloud->GetSeed() + 1);
    if (m_tracer) m_tracer->Reset();
  }

  if (InputSystem::GetKeyDown(KeyCode::ONE)) m_cloud->SetIntegrator(Integrator::ForwardEuler);
  if (InputSystem::GetKeyDown(KeyCode::TWO)) m_cloud->SetIntegrator(Integrator::SemiImplicitEuler);
  if (InputSystem::GetKeyDown(KeyCode::THREE)) m_cloud->SetIntegrator(Integrator::VelocityVerlet);
  if (InputSystem::GetKeyDown(KeyCode::FOUR)) m_cloud->SetIntegrator(Integrator::RungeKutta4);

  if (InputSystem::GetKeyDown(KeyCode::LEFT_BRACKET)) m_cloud->SetStep(m_cloud->GetStep() * 0.5);
  if (InputSystem::GetKeyDown(KeyCode::RIGHT_BRACKET)) m_cloud->SetStep(m_cloud->GetStep() * 2.);

  if (InputSystem::GetKeyDown(KeyCode::COMMA)) m_cloud->SetTimeScale(m_cloud->GetTimeScale() - timeScaleStep);
  if (InputSystem::GetKeyDown(KeyCode::PERIOD)) m_cloud->SetTimeScale(m_cloud->GetTimeScale() + timeScaleStep);
}
