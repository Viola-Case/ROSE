#line 2 "examples/orbits/pointcloud.cpp"
/**

  @file       pointcloud.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "pointcloud.h"
#include "centralbody.h"
#include "simcommon.h"
#include "simdraw.h"

#include <SDL3/SDL.h>
#include <chrono>
#include <random>

using namespace ROSE;

namespace {
  /*! Above this many substeps in one frame the simulation is losing; drop the backlog instead of spiralling. */
  constexpr int maxSubstepsPerFrame { 240 };

  /*! Ignore frame times longer than this. A stall should not fire a thousand steps the moment focus returns. */
  constexpr double maxFrameSeconds { 0.05 };

  constexpr size_t minCount { 1 };
  constexpr size_t maxCount { 200000 };

  constexpr double minStep { 1e-5 };
  constexpr double maxStep { 0.5 };

  /*! Colour ramp: cold blue below circular speed, white at circular, hot orange above it. */
  struct Stop {
    double t;
    double r, g, b;
  };
  constexpr Stop ramp[] {
    {  0.,  60., 110., 255. },
    { 0.5, 235., 240., 255. },
    {  1., 255., 118.,  40. },
  };

  constexpr size_t rampStops { sizeof(ramp) / sizeof(ramp[0]) };

  void SampleRamp(double _t, double &_r, double &_g, double &_b) noexcept {
    _t = orb::Clamp(_t, 0., 1.);

    size_t hi = 1;
    while (hi + 1 < rampStops && _t > ramp[hi].t)
      ++hi;

    const Stop &a = ramp[hi - 1];
    const Stop &b = ramp[hi];
    const double u = orb::Clamp((_t - a.t) / (b.t - a.t), 0., 1.);
    _r = a.r + (b.r - a.r) * u;
    _g = a.g + (b.g - a.g) * u;
    _b = a.b + (b.b - a.b) * u;
  }
} // namespace

const char *IntegratorName(Integrator _i) noexcept {
  switch (_i) {
    case Integrator::ForwardEuler: return "Forward Euler";
    case Integrator::SemiImplicitEuler: return "Semi-implicit Euler";
    case Integrator::VelocityVerlet: return "Velocity Verlet";
    case Integrator::RungeKutta4: return "Runge-Kutta 4";
    default: return "?";
  }
}

const char *IntegratorNote(Integrator _i) noexcept {
  switch (_i) {
    case Integrator::ForwardEuler: return "1st order, not symplectic - gains energy, orbits spiral outward";
    case Integrator::SemiImplicitEuler: return "1st order, symplectic - energy oscillates but does not drift";
    case Integrator::VelocityVerlet: return "2nd order, symplectic, time reversible - bounded energy error";
    case Integrator::RungeKutta4: return "4th order, NOT symplectic - very accurate, still drifts secularly";
    default: return "";
  }
}

void PointCloud::Unpack(const ParamView &_view) {
  const int count = _view.GetInt("count", static_cast<int>(m_count));
  m_count = count > 0 ? static_cast<size_t>(count) : m_count;
  m_count = m_count < minCount ? minCount : (m_count > maxCount ? maxCount : m_count);

  m_seed = static_cast<uint32_t>(_view.GetInt("seed", static_cast<int>(m_seed)));
  m_radiusMin = _view.GetDouble("radiusMin", m_radiusMin);
  m_radiusMax = _view.GetDouble("radiusMax", m_radiusMax);
  m_speedJitter = orb::Clamp(_view.GetDouble("speedJitter", m_speedJitter), 0., 1.);
  m_inclinationSpread = orb::Clamp(_view.GetDouble("inclinationSpread", m_inclinationSpread), 0., 1.);
  m_step = orb::Clamp(_view.GetDouble("step", m_step), minStep, maxStep);
  m_timeScale = orb::Clamp(_view.GetDouble("timeScale", m_timeScale), 0., 100.);

  const int integrator = _view.GetInt("integrator", static_cast<int>(m_integrator));
  if (integrator >= 0 && integrator < static_cast<int>(Integrator::Count))
    m_integrator = static_cast<Integrator>(integrator);

  if (m_radiusMin <= 0.) m_radiusMin = 1.;
  if (m_radiusMax < m_radiusMin) m_radiusMax = m_radiusMin;
}

void PointCloud::OnStart() {
  auto *window = static_cast<SDL_Window *>(m_object->GetScene().GetApplication().GetWindow());
  m_renderer = window ? SDL_GetRenderer(window) : nullptr;

  Scene &scene = m_object->GetScene();
  m_camera = orb::FindInScene<OrbitCamera>(scene);
  m_body = orb::FindInScene<CentralBody>(scene);

  if (!m_camera) ROSE_LOG_ERROR("PointCloud: no OrbitCamera in the scene; nothing will be drawn.");
  if (!m_body) {
    ROSE_LOG_ERROR("PointCloud: no CentralBody in the scene; there is nothing to orbit.");
    return;
  }

  BuildPalette();
  Reseed();
}

void PointCloud::Allocate(size_t _n) noexcept {
  m_pos.resize(_n);
  m_vel.resize(_n);
  m_acc.resize(_n);
  m_rkPos.resize(_n);
  for (size_t k = 0; k < 4; ++k) {
    m_rkA[k].resize(_n);
    m_rkV[k].resize(_n);
  }
}

void PointCloud::Reseed() noexcept {
  if (!m_body) return;

  m_center = m_body->GetCenter();
  m_gm = m_body->GetGM();
  m_softening = m_body->GetSoftening();

  Allocate(m_count);

  /* mt19937 rather than random_device: a fixed seed has to reproduce the same cloud exactly, otherwise comparing
   * two integrators on "the same" initial conditions means nothing. */
  std::mt19937 gen(m_seed);
  std::uniform_real_distribution<double> unit(0., 1.);

  /* Each particle gets its own orbital plane, described by the plane's unit normal n. Sampling n uniformly in
   * cos(θ) over a polar cap gives a distribution that is uniform per unit solid angle: spread 0 collapses the cap
   * to the +y pole (every orbit in the xz plane, a flat disc), spread 1 opens it to the whole sphere (an
   * isotropic swarm). Anything between is a thickened disc. */
  const double cosLimit = 1. - 2. * m_inclinationSpread;

  for (size_t i = 0; i < m_count; ++i) {
    /* Uniform in radius, not in volume: that puts more particles per unit area near the centre, which is roughly
     * what a real disc looks like and reads better than a hollow-looking uniform-density shell. */
    const double r = m_radiusMin + (m_radiusMax - m_radiusMin) * unit(gen);

    const double cz = cosLimit + (1. - cosLimit) * unit(gen);
    const double sz = math::Sqrt(orb::Clamp(1. - cz * cz, 0., 1.));
    const double phi = orb::twoPi * unit(gen);
    const Vec3d n { sz * math::Cos(phi), cz, sz * math::Sin(phi) };

    /* A random phase within that plane: e1, e2 and n form an orthonormal frame, so any combination of e1 and e2
     * is a unit vector lying in the orbital plane. */
    const Vec3d e1 = orb::AnyPerpendicular(n);
    const Vec3d e2 = n.cross(e1);
    const double psi = orb::twoPi * unit(gen);
    const Vec3d radial = e1 * math::Cos(psi) + e2 * math::Sin(psi);

    /* n × radial is the prograde tangent: unit length, in-plane, perpendicular to the radius. */
    const Vec3d tangent = n.cross(radial);

    const double vc = m_body->CircularSpeed(r);
    const double tangentialScale = 1. + m_speedJitter * (2. * unit(gen) - 1.);
    const double radialScale = 0.5 * m_speedJitter * (2. * unit(gen) - 1.);

    /* Perturbing the tangential speed alone would leave every particle starting at an apsis of its own ellipse,
     * which shows up as a visible ring at the seed radius. The radial kick spreads the starting true anomalies
     * out so the cloud looks - and is - genuinely randomised. */
    m_pos[i] = m_center + radial * r;
    m_vel[i] = tangent * (vc * tangentialScale) + radial * (vc * radialScale);
  }

  /* Velocity Verlet carries acceleration across steps, so it has to start with a valid one. */
  Accelerate(m_pos, m_acc);

  m_accumulator = 0.;
  m_stats = SimStats {};
  Measure();
  m_stats.energy0 = m_stats.energy;
  m_stats.angularMomentum0 = m_stats.angularMomentum;
}

void PointCloud::SetCount(size_t _n) noexcept {
  _n = _n < minCount ? minCount : (_n > maxCount ? maxCount : _n);
  if (_n == m_count) return;
  m_count = _n;
  Reseed();
}

void PointCloud::SetSeed(uint32_t _seed) noexcept {
  m_seed = _seed;
  Reseed();
}

void PointCloud::SetIntegrator(Integrator _i) noexcept {
  if (_i < Integrator::ForwardEuler || _i >= Integrator::Count || _i == m_integrator) return;
  m_integrator = _i;
  /* Verlet needs a current acceleration; the others overwrite it anyway, so this is unconditional and cheap. */
  if (!m_pos.empty()) Accelerate(m_pos, m_acc);
}

void PointCloud::SetStep(double _h) noexcept {
  m_step = orb::Clamp(_h, minStep, maxStep);
}

void PointCloud::SetTimeScale(double _s) noexcept {
  m_timeScale = orb::Clamp(_s, 0., 100.);
}

void PointCloud::SetSpeedJitter(double _j) noexcept {
  m_speedJitter = orb::Clamp(_j, 0., 1.);
  Reseed();
}

void PointCloud::SetInclinationSpread(double _s) noexcept {
  m_inclinationSpread = orb::Clamp(_s, 0., 1.);
  Reseed();
}

double PointCloud::GetEnergyDrift() const noexcept {
  const double e0 = m_stats.energy0;
  return e0 != 0. ? (m_stats.energy - e0) / (e0 < 0. ? -e0 : e0) : 0.;
}

double PointCloud::GetAngularMomentumDrift() const noexcept {
  const double l0 = m_stats.angularMomentum0;
  return l0 != 0. ? (m_stats.angularMomentum - l0) / l0 : 0.;
}

void PointCloud::Accelerate(const List<Vec3d> &_pos, List<Vec3d> &_acc) const noexcept {
  const double cx = m_center.x, cy = m_center.y, cz = m_center.z;
  const double gm = m_gm;
  const double eps2 = m_softening * m_softening;
  const size_t n = _pos.size();

  const Vec3d *src = _pos.data();
  Vec3d *dst = _acc.data();

  for (size_t i = 0; i < n; ++i) {
    const double dx = cx - src[i].x;
    const double dy = cy - src[i].y;
    const double dz = cz - src[i].z;
    const double r2 = dx * dx + dy * dy + dz * dz + eps2;
    const double invR = 1. / math::Sqrt(r2);
    const double s = gm * invR * invR * invR;   //!< GM / (r² + ε²)^{3/2}
    dst[i] = { dx * s, dy * s, dz * s };
  }
}

void PointCloud::Step(double _h) noexcept {
  const size_t n = m_pos.size();
  Vec3d *pos = m_pos.data();
  Vec3d *vel = m_vel.data();
  Vec3d *acc = m_acc.data();

  switch (m_integrator) {
    case Integrator::ForwardEuler: {
      /* Both updates read the *old* state, hence the copy of v before it is overwritten. This is the textbook
       * wrong answer and it is here on purpose: it is unstable for oscillatory systems at any step size, and the
       * cloud visibly unwinds. */
      Accelerate(m_pos, m_acc);
      for (size_t i = 0; i < n; ++i) {
        const Vec3d v = vel[i];
        pos[i] += v * _h;
        vel[i] += acc[i] * _h;
      }
    } break;

    case Integrator::SemiImplicitEuler: {
      /* One character of difference from the above - position integrates against the *new* velocity - and it
       * becomes symplectic. This is what Core's `Motion` behavior does. */
      Accelerate(m_pos, m_acc);
      for (size_t i = 0; i < n; ++i) {
        vel[i] += acc[i] * _h;
        pos[i] += vel[i] * _h;
      }
    } break;

    case Integrator::VelocityVerlet: {
      /* Kick, drift, kick: half a velocity step on the old acceleration, a full position step, then the other
       * half on the new one. One force evaluation per step, same as Euler, for second-order accuracy. */
      const double half = 0.5 * _h;
      for (size_t i = 0; i < n; ++i) {
        vel[i] += acc[i] * half;
        pos[i] += vel[i] * _h;
      }
      Accelerate(m_pos, m_acc);
      for (size_t i = 0; i < n; ++i)
        vel[i] += acc[i] * half;
    } break;

    case Integrator::RungeKutta4: {
      /* Classical RK4 on the first-order system y = (r, v), f(y) = (v, a(r)). Four force evaluations per step,
       * so roughly 4x the cost of Verlet - which buys two extra orders of local accuracy but, because RK4 is not
       * symplectic, does not buy a bounded energy error. Over long runs it loses to Verlet on exactly the thing
       * the HUD is plotting. */
      const double h2 = _h * 0.5;
      const double h6 = _h / 6.;

      Vec3d *rkPos = m_rkPos.data();
      Vec3d *a1 = m_rkA[0].data(), *a2 = m_rkA[1].data(), *a3 = m_rkA[2].data(), *a4 = m_rkA[3].data();
      Vec3d *v1 = m_rkV[0].data(), *v2 = m_rkV[1].data(), *v3 = m_rkV[2].data(), *v4 = m_rkV[3].data();

      Accelerate(m_pos, m_rkA[0]);
      for (size_t i = 0; i < n; ++i) {
        v1[i] = vel[i];
        rkPos[i] = pos[i] + v1[i] * h2;
      }

      Accelerate(m_rkPos, m_rkA[1]);
      for (size_t i = 0; i < n; ++i) {
        v2[i] = vel[i] + a1[i] * h2;
        rkPos[i] = pos[i] + v2[i] * h2;
      }

      Accelerate(m_rkPos, m_rkA[2]);
      for (size_t i = 0; i < n; ++i) {
        v3[i] = vel[i] + a2[i] * h2;
        rkPos[i] = pos[i] + v3[i] * _h;
      }

      Accelerate(m_rkPos, m_rkA[3]);
      for (size_t i = 0; i < n; ++i) {
        v4[i] = vel[i] + a3[i] * _h;
        pos[i] += (v1[i] + v2[i] * 2. + v3[i] * 2. + v4[i]) * h6;
        vel[i] += (a1[i] + a2[i] * 2. + a3[i] * 2. + a4[i]) * h6;
      }

      /* Leave m_acc consistent with the new positions in case the integrator is switched to Verlet next frame. */
      Accelerate(m_pos, m_acc);
    } break;

    default: break;
  }

  m_stats.time += _h;
  ++m_stats.steps;
}

void PointCloud::Measure() noexcept {
  const size_t n = m_pos.size();
  const Vec3d *pos = m_pos.data();
  const Vec3d *vel = m_vel.data();
  const double eps2 = m_softening * m_softening;

  double kinetic { 0. };
  double potential { 0. };
  Vec3d angular {};

  for (size_t i = 0; i < n; ++i) {
    const Vec3d rel = pos[i] - m_center;
    const Vec3d &v = vel[i];
    kinetic += 0.5 * v.dot(v);
    potential += -m_gm / math::Sqrt(rel.dot(rel) + eps2);
    angular += rel.cross(v);
  }

  m_stats.kinetic = kinetic;
  m_stats.potential = potential;
  m_stats.energy = kinetic + potential;
  m_stats.angularMomentum = orb::Length(angular);
}

void PointCloud::FrameUpdate() {
  if (m_body) {
    m_center = m_body->GetCenter();
    m_gm = m_body->GetGM();
    m_softening = m_body->GetSoftening();
  }

  m_stats.substeps = 0;
  m_stats.stepMilliseconds = 0.;

  if (!m_paused && !m_pos.empty()) {
    /* The engine declares a FixedUpdate hook but never calls it (see docs/internal/scene-object-behavior.md), so
     * the fixed-step loop lives here. Decoupling the physics step from the frame time is not optional for this
     * example: if h tracked the frame rate, the energy drift on the HUD would be measuring the display and not
     * the integrator. */
    const auto begin = std::chrono::high_resolution_clock::now();

    m_accumulator += math::Min(Time::deltaTime, maxFrameSeconds) * m_timeScale;

    int taken = 0;
    while (m_accumulator >= m_step && taken < maxSubstepsPerFrame) {
      Step(m_step);
      m_accumulator -= m_step;
      ++taken;
    }
    /* If the budget ran out the simulation cannot keep up at this time scale. Throw the remainder away rather
     * than carry it forward, which would only make the next frame worse. */
    if (taken == maxSubstepsPerFrame) m_accumulator = 0.;

    const auto end = std::chrono::high_resolution_clock::now();
    m_stats.substeps = taken;
    m_stats.stepMilliseconds = std::chrono::duration<double, std::milli>(end - begin).count();

    if (taken > 0) Measure();
  }

  Draw();
}

void PointCloud::BuildPalette() noexcept {
  for (size_t c = 0; c < colorBins; ++c) {
    double r {}, g {}, b {};
    SampleRamp(static_cast<double>(c) / static_cast<double>(colorBins - 1), r, g, b);
    for (size_t s = 0; s < shadeBins; ++s) {
      /* Bin 0 is the most distant and keeps 1/shadeBins of its brightness; the nearest bin keeps all of it. */
      const double shade = static_cast<double>(s + 1) / static_cast<double>(shadeBins);
      uint8_t *out = m_bucketRGB[c * shadeBins + s];
      out[0] = static_cast<uint8_t>(orb::Clamp(r * shade, 0., 255.));
      out[1] = static_cast<uint8_t>(orb::Clamp(g * shade, 0., 255.));
      out[2] = static_cast<uint8_t>(orb::Clamp(b * shade, 0., 255.));
    }
  }
}

void PointCloud::Draw() noexcept {
  if (!m_renderer || !m_camera) return;

  /* clear() keeps the capacity, so the steady state does not touch the allocator. */
  for (auto &bucket : m_buckets)
    bucket.clear();

  const double width = m_camera->GetViewWidth();
  const double height = m_camera->GetViewHeight();
  const double reference = m_camera->GetDistance();   //!< depth at which a particle is drawn at full brightness

  const size_t n = m_pos.size();
  const Vec3d *pos = m_pos.data();
  const Vec3d *vel = m_vel.data();
  size_t drawn { 0 };

  for (size_t i = 0; i < n; ++i) {
    const Projection p = m_camera->Project(pos[i]);
    if (!p.visible) continue;
    if (p.point.x < 0.f || p.point.y < 0.f || p.point.x >= width || p.point.y >= height) continue;

    /* Colour by how fast the particle is moving relative to a circular orbit at its current radius. Below 1 it is
     * inside a bound ellipse near apoapsis, at 1 it is circular, above √2 it is unbound and leaving for good. */
    const Vec3d rel = pos[i] - m_center;
    const double r = orb::Length(rel);
    const double vc = m_body ? m_body->CircularSpeed(r) : 1.;
    const double ratio = vc > 0. ? orb::Length(vel[i]) / vc : 0.;

    const double t = orb::Clamp((ratio - 0.4) / 1.2, 0., 1.);
    auto colorBin = static_cast<size_t>(t * static_cast<double>(colorBins - 1) + 0.5);

    const double shade = orb::Clamp(reference / p.depth, 0., 1.);
    auto shadeBin = static_cast<size_t>(shade * static_cast<double>(shadeBins));
    if (shadeBin >= shadeBins) shadeBin = shadeBins - 1;

    m_buckets[colorBin * shadeBins + shadeBin].push_back(p.point);
    ++drawn;
  }

  for (size_t b = 0; b < bucketCount; ++b) {
    const List<ScreenPoint> &bucket = m_buckets[b];
    if (bucket.empty()) continue;
    orb::SetColor(m_renderer, m_bucketRGB[b][0], m_bucketRGB[b][1], m_bucketRGB[b][2]);
    orb::DrawPoints(m_renderer, bucket.data(), bucket.size());
  }

  m_stats.drawn = drawn;
}
