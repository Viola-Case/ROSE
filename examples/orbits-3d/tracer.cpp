#line 2 "examples/orbits/tracer.cpp"
/**

  @file       tracer.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "tracer.h"
#include "centralbody.h"
#include "orbitcamera.h"
#include "simcommon.h"
#include "simdraw.h"

#include <SDL3/SDL.h>

using namespace ROSE;

namespace {
  constexpr size_t maxTrailLength { 20000 };
  constexpr double markerRadius { 2.5 };
} // namespace

void Tracer::Unpack(const ParamView &_view) {
  m_radius = _view.GetDouble("radius", m_radius);
  m_inclination = _view.GetDouble("inclination", m_inclination);

  const int trail = _view.GetInt("trailLength", static_cast<int>(m_trailLength));
  m_trailLength = trail > 0 ? static_cast<size_t>(trail) : 0;
  if (m_trailLength > maxTrailLength) m_trailLength = maxTrailLength;

  if (m_radius <= 0.) m_radius = 1.;
}

void Tracer::OnStart() {
  auto *window = static_cast<SDL_Window *>(m_object->GetScene().GetApplication().GetWindow());
  m_renderer = window ? SDL_GetRenderer(window) : nullptr;

  Scene &scene = m_object->GetScene();
  m_camera = orb::FindInScene<OrbitCamera>(scene);
  m_body = orb::FindInScene<CentralBody>(scene);

  /* The Motion has to be a sibling on this same object - it is what carries the state this behavior steers.
   * `m_behaviors` is keyed by type ID, so there is at most one of them and FindBehavior always finds it. */
  m_motion = m_object->FindBehavior<Motion>();
  if (!m_motion) ROSE_LOG_ERROR("Tracer: no Motion on '{}'; add one in the scene JSON.", m_object->GetName());
  if (!m_body) ROSE_LOG_ERROR("Tracer: no CentralBody in the scene; there is nothing to orbit.");

  m_trail.reserve(m_trailLength);
  m_trailOrdered.reserve(m_trailLength);

  Reset();
}

void Tracer::Reset() noexcept {
  if (!m_motion || !m_body) return;

  const Vec3d center = m_body->GetCenter();

  /* Seed on a circular orbit tilted out of the xz plane by `inclination`, rotating about the x axis. Written out
   * by hand because `Quat` has no vector-rotation operator yet (docs/internal/math.md). */
  const double ci = math::Cos(m_inclination);
  const double si = math::Sin(m_inclination);

  const Vec3d radial { m_radius, 0., 0. };
  const Vec3d tangent { 0., si, ci };   //!< +z tangent tilted toward +y; still unit length and perpendicular to radial

  m_object->transform.position = center + radial;
  m_motion->SetVelocity(tangent * m_body->CircularSpeed(m_radius));
  m_motion->SetAcceleration(m_body->AccelerationAt(m_object->transform.position));

  m_trail.clear();
  m_trailOrdered.clear();
  m_trailHead = 0;
  m_trailFilled = 0;

  const Vec3d &v = m_motion->GetVelocity();
  m_energy = 0.5 * v.dot(v) + m_body->PotentialAt(m_object->transform.position);
  m_energy0 = m_energy;
}

double Tracer::GetEnergyDrift() const noexcept {
  return m_energy0 != 0. ? (m_energy - m_energy0) / (m_energy0 < 0. ? -m_energy0 : m_energy0) : 0.;
}

void Tracer::FrameUpdate() {
  if (!m_motion || !m_body) return;

  const Vec3d &position = m_object->transform.position;

  /* Feed the field to the engine's integrator. See the class comment: whether Motion consumes this on the current
   * frame or the next one depends on hash-map ordering, and either way it is what the engine gives you. */
  m_motion->SetAcceleration(m_body->AccelerationAt(position));

  const Vec3d &v = m_motion->GetVelocity();
  m_energy = 0.5 * v.dot(v) + m_body->PotentialAt(position);

  if (m_trailLength > 0) {
    if (m_trail.size() < m_trailLength) {
      m_trail.push_back(position);
      m_trailFilled = m_trail.size();
      m_trailHead = m_trail.size() % m_trailLength;
    } else {
      m_trail[m_trailHead] = position;
      m_trailHead = (m_trailHead + 1) % m_trailLength;
      m_trailFilled = m_trailLength;
    }
  }

  if (!m_renderer || !m_camera) return;

  if (m_trailFilled >= 2) {
    /* Unroll the ring into chronological order for the polyline. Oldest sample first: once the buffer is full
     * that is the one at the head, which is about to be overwritten next frame. */
    m_trailOrdered.clear();
    const size_t start = m_trailFilled == m_trailLength ? m_trailHead : 0;
    for (size_t i = 0; i < m_trailFilled; ++i)
      m_trailOrdered.push_back(m_trail[(start + i) % m_trailFilled]);

    orb::SetColor(m_renderer, 60, 150, 140);
    orb::DrawProjectedPath(m_renderer, *m_camera, m_trailOrdered.data(), m_trailOrdered.size());
  }

  const Projection p = m_camera->Project(position);
  if (!p.visible) return;

  orb::SetColor(m_renderer, 120, 255, 235);
  orb::FillDisc(m_renderer, p.point.x, p.point.y, static_cast<float>(markerRadius));
}
