#line 2 "examples/orbits/orbitcamera.cpp"
/**

  @file       orbitcamera.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "orbitcamera.h"
#include "simcommon.h"

#include <SDL3/SDL.h>

using namespace ROSE;

namespace {
  constexpr double nearPlane { 0.05 };
  constexpr double minDistance { 12. };
  constexpr double maxDistance { 6000. };
  constexpr double turnRate { 1.4 };    //!< radians per second on the arrow keys
  constexpr double zoomRate { 1.6 };    //!< fraction of the current distance per second
  constexpr double pitchLimit { orb::pi * 0.5 - 0.02 };
  constexpr double degToRad { orb::pi / 180. };
} // namespace

void OrbitCamera::Unpack(const ParamView &_view) {
  m_yaw = _view.GetDouble("yaw", m_yaw);
  m_pitch = orb::Clamp(_view.GetDouble("pitch", m_pitch), -pitchLimit, pitchLimit);
  m_distance = orb::Clamp(_view.GetDouble("distance", m_distance), minDistance, maxDistance);
  m_fov = orb::Clamp(_view.GetDouble("fov", m_fov), 5., 160.);
  m_spinRate = _view.GetDouble("spinRate", m_spinRate);
  m_spin = _view.GetBool("spin", m_spin);

  m_yaw0 = m_yaw;
  m_pitch0 = m_pitch;
  m_distance0 = m_distance;
}

void OrbitCamera::OnStart() {
  Rebuild();
}

void OrbitCamera::ResetView() noexcept {
  m_yaw = m_yaw0;
  m_pitch = m_pitch0;
  m_distance = m_distance0;
  Rebuild();
}

void OrbitCamera::FrameUpdate() {
  const double dt = math::Min(Time::deltaTime, 0.1);

  if (InputSystem::GetKey(KeyCode::LEFT)) m_yaw -= turnRate * dt;
  if (InputSystem::GetKey(KeyCode::RIGHT)) m_yaw += turnRate * dt;
  if (InputSystem::GetKey(KeyCode::UP)) m_pitch += turnRate * dt;
  if (InputSystem::GetKey(KeyCode::DOWN)) m_pitch -= turnRate * dt;
  if (InputSystem::GetKey(KeyCode::PAGE_UP)) m_distance *= 1. - zoomRate * dt;
  if (InputSystem::GetKey(KeyCode::PAGE_DOWN)) m_distance *= 1. + zoomRate * dt;
  if (InputSystem::GetKeyDown(KeyCode::HOME)) ResetView();
  if (InputSystem::GetKeyDown(KeyCode::O)) ToggleSpin();

  if (m_spin) m_yaw += m_spinRate * dt;

  /* Keep yaw in [0, 2π) so it cannot accumulate enough magnitude to start losing precision in Sin/Cos over a long
   * session, and keep pitch off the poles where the up vector is undefined. */
  while (m_yaw >= orb::twoPi)
    m_yaw -= orb::twoPi;
  while (m_yaw < 0.)
    m_yaw += orb::twoPi;
  m_pitch = orb::Clamp(m_pitch, -pitchLimit, pitchLimit);
  m_distance = orb::Clamp(m_distance, minDistance, maxDistance);

  Rebuild();
}

void OrbitCamera::Rebuild() noexcept {
  if (auto *window = static_cast<SDL_Window *>(m_object->GetScene().GetApplication().GetWindow())) {
    int w {}, h {};
    SDL_GetWindowSize(window, &w, &h);
    if (w > 0 && h > 0) {
      m_halfW = w * 0.5;
      m_halfH = h * 0.5;
    }
  }

  /* Pinhole model: a point at distance z lands focalPx * (x/z) pixels off centre, so focalPx is the distance at
   * which the half-height of the viewport subtends half the vertical field of view. */
  m_focalPx = m_halfH / math::Tan(m_fov * degToRad * 0.5);

  const double cp = math::Cos(m_pitch);
  const double sp = math::Sin(m_pitch);
  const double cy = math::Cos(m_yaw);
  const double sy = math::Sin(m_yaw);

  /* Direction from the target out to the eye. Positive pitch puts the eye above the scene, so forward - which is
   * the other way - points down at it. */
  const Vec3d eyeDir { cp * cy, sp, cp * sy };

  const Vec3d &target = m_object->transform.position;
  m_eye = target + eyeDir * m_distance;
  m_forward = orb::Negated(eyeDir);
  m_right = orb::Unit(m_forward.cross({ 0., 1., 0. }));
  m_up = m_right.cross(m_forward);
}

Projection OrbitCamera::Project(const Vec3d &_world) const noexcept {
  const Vec3d rel = _world - m_eye;
  const double z = rel.dot(m_forward);
  if (z <= nearPlane) return {};

  const double invZ = 1. / z;
  Projection p {};
  p.depth = z;
  p.visible = true;
  p.point.x = static_cast<float>(m_halfW + m_focalPx * rel.dot(m_right) * invZ);
  /* Screen y grows downward, world y grows upward. */
  p.point.y = static_cast<float>(m_halfH - m_focalPx * rel.dot(m_up) * invZ);
  return p;
}

double OrbitCamera::PixelsPerUnit(double _depth) const noexcept {
  return _depth > nearPlane ? m_focalPx / _depth : 0.;
}
