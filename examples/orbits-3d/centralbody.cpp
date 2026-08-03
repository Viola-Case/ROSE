#line 2 "examples/orbits/centralbody.cpp"
/**

  @file       centralbody.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "centralbody.h"
#include "orbitcamera.h"
#include "simcommon.h"
#include "simdraw.h"

#include <SDL3/SDL.h>

using namespace ROSE;

namespace {
  constexpr size_t ringSegments { 128 };
} // namespace

void CentralBody::Unpack(const ParamView &_view) {
  m_gm = _view.GetDouble("gm", m_gm);
  m_softening = _view.GetDouble("softening", m_softening);
  m_radius = _view.GetDouble("radius", m_radius);
  m_ringRadius = _view.GetDouble("ringRadius", m_ringRadius);

  if (m_gm <= 0.) {
    ROSE_LOG_ERROR("CentralBody: gm must be positive, got {}. Falling back to 1.", m_gm);
    m_gm = 1.;
  }
  if (m_softening < 0.) m_softening = 0.;
}

void CentralBody::OnStart() {
  auto *window = static_cast<SDL_Window *>(m_object->GetScene().GetApplication().GetWindow());
  m_renderer = window ? SDL_GetRenderer(window) : nullptr;

  m_camera = orb::FindInScene<OrbitCamera>(m_object->GetScene());
  if (!m_camera) ROSE_LOG_ERROR("CentralBody: no OrbitCamera in the scene; nothing will be drawn.");
}

Vec3d CentralBody::AccelerationAt(const Vec3d &_at) const noexcept {
  const Vec3d d = GetCenter() - _at;
  const double r2 = d.dot(d) + m_softening * m_softening;
  const double invR = 1. / math::Sqrt(r2);
  return d * (m_gm * invR * invR * invR);
}

double CentralBody::PotentialAt(const Vec3d &_at) const noexcept {
  const Vec3d d = GetCenter() - _at;
  return -m_gm / math::Sqrt(d.dot(d) + m_softening * m_softening);
}

double CentralBody::CircularSpeed(double _r) const noexcept {
  if (_r <= 0.) return 0.;
  /* Balance the softened acceleration against the centripetal requirement v²/r:
   *   GM r / (r² + ε²)^{3/2} = v² / r   ⇒   v = r √( GM / (r² + ε²)^{3/2} ). */
  const double s = _r * _r + m_softening * m_softening;
  return _r * math::Sqrt(m_gm / (s * math::Sqrt(s)));
}

void CentralBody::FrameUpdate() {
  if (!m_renderer || !m_camera) return;

  const Vec3d &center = GetCenter();

  if (m_ringRadius > 0.) {
    /* A reference circle in the xz plane. Without it a rotating point cloud on a black background gives the eye
     * nothing to lock onto and the camera motion reads as the cloud deforming. */
    Vec3d ring[ringSegments + 1];
    for (size_t i = 0; i <= ringSegments; ++i) {
      const double a = orb::twoPi * static_cast<double>(i) / static_cast<double>(ringSegments);
      ring[i] = center + Vec3d { math::Cos(a) * m_ringRadius, 0., math::Sin(a) * m_ringRadius };
    }
    orb::SetColor(m_renderer, 34, 44, 68);
    orb::DrawProjectedPath(m_renderer, *m_camera, ring, ringSegments + 1);
  }

  const Projection p = m_camera->Project(center);
  if (!p.visible) return;

  const float screenRadius = static_cast<float>(m_radius * m_camera->PixelsPerUnit(p.depth));

  /* A cheap two-pass glow: a dim wide disc under a bright core. The software renderer has no blending set up, so
   * this is opaque - it reads as a corona rather than a real bloom, which is enough to sell "this is the mass". */
  orb::SetColor(m_renderer, 92, 72, 30);
  orb::FillDisc(m_renderer, p.point.x, p.point.y, screenRadius * 1.9f);
  orb::SetColor(m_renderer, 255, 226, 150);
  orb::FillDisc(m_renderer, p.point.x, p.point.y, screenRadius);
}
