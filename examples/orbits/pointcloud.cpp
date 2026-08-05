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

#include <SDL3/SDL.h>
#include <cstddef>
#include <random>

using namespace ROSE;

/* ScreenPoint exists so the header does not have to include SDL. It only earns its keep if an array of them can
 * go straight to SDL without a per-point copy, which is what these checks guard. If SDL_FPoint ever changes, this
 * fails to compile here rather than silently drawing garbage. */
static_assert(sizeof(ScreenPoint) == sizeof(SDL_FPoint));
static_assert(alignof(ScreenPoint) == alignof(SDL_FPoint));
static_assert(offsetof(ScreenPoint, x) == offsetof(SDL_FPoint, x));
static_assert(offsetof(ScreenPoint, y) == offsetof(SDL_FPoint, y));

namespace {
  /* `math::PI` is initialised from a float literal, so it only carries float precision - see
   * docs/internal/known-issues.md #8. Nothing here needs the extra digits, but it costs nothing to be right. */
  constexpr double pi { 3.14159265358979323846 };
  constexpr double twoPi { 2. * pi };

  /*! Above this many substeps in one frame the simulation is losing; drop the backlog rather than spiral. */
  constexpr int maxSubstepsPerFrame { 240 };

  /*! Ignore frame times longer than this, so a stall does not fire a thousand steps when focus returns. */
  constexpr double maxFrameSeconds { 0.05 };

  constexpr double centralRadius { 5. };

  void FillDisc(SDL_Renderer *_renderer, float _cx, float _cy, float _radius) noexcept {
    const int rad = static_cast<int>(_radius);
    const float r2 = _radius * _radius;
    for (int dy = -rad; dy <= rad; ++dy) {
      const float y = static_cast<float>(dy);
      const float halfSpan = math::Sqrt(r2 - y * y);
      SDL_RenderLine(_renderer, _cx - halfSpan, _cy + y, _cx + halfSpan, _cy + y);
    }
  }
} // namespace

void PointCloud::Unpack(const ParamView &_view) {
  const int count = _view.GetInt("count", static_cast<int>(m_count));
  if (count > 0) m_count = static_cast<size_t>(count);

  m_seed = static_cast<uint32_t>(_view.GetInt("seed", static_cast<int>(m_seed)));
  m_gm = _view.GetDouble("gm", m_gm);
  m_softening = _view.GetDouble("softening", m_softening);
  m_radiusMin = _view.GetDouble("radiusMin", m_radiusMin);
  m_radiusMax = _view.GetDouble("radiusMax", m_radiusMax);
  m_speedJitter = _view.GetDouble("speedJitter", m_speedJitter);
  m_step = _view.GetDouble("step", m_step);

  if (m_gm <= 0.) m_gm = 1.;
  if (m_softening < 0.) m_softening = 0.;
  if (m_radiusMin <= 0.) m_radiusMin = 1.;
  if (m_radiusMax < m_radiusMin) m_radiusMax = m_radiusMin;
  if (m_step <= 0.) m_step = 0.002;
}

void PointCloud::OnStart() {
  Window *window = m_object->GetScene().GetApplication().GetWindow();
  if (!window || !window->IsValid()) {
    ROSE_LOG_ERROR("PointCloud: no window; nothing will be drawn.");
    return;
  }
  m_renderer = SDL_GetRenderer(static_cast<SDL_Window *>(window->GetHandle()));

  const auto size = window->GetSize();
  m_width = size.x;
  m_height = size.y;

  /* Everything is in window pixels, so the attractor goes at the middle of the screen. It lives in the object's
   * own Transform rather than in a member of this behavior, so moving the object moves the whole field. */
  m_object->transform.position = { m_width * 0.5, m_height * 0.5, 0. };

  Reseed();
}

void PointCloud::Reseed() noexcept {
  m_pos.resize(m_count);
  m_vel.resize(m_count);
  m_screen.resize(m_count);

  /* mt19937 with a fixed seed rather than random_device: the same seed has to rebuild the same cloud, so that
   * pressing R is a reset and not a reroll. */
  std::mt19937 gen(m_seed);
  std::uniform_real_distribution<double> unit(0., 1.);

  const double eps2 = m_softening * m_softening;

  for (size_t i = 0; i < m_count; ++i) {
    const double r = m_radiusMin + (m_radiusMax - m_radiusMin) * unit(gen);
    const double angle = twoPi * unit(gen);

    const Vec3d radial { math::Cos(angle), math::Sin(angle), 0. };
    const Vec3d tangent { -radial.y, radial.x, 0. };

    /* Speed of a circular orbit at this radius, accounting for the softening: balancing the softened
     * acceleration against the centripetal requirement v²/r gives v = r √( GM / (r² + ε²)^{3/2} ). */
    const double s = r * r + eps2;
    const double vc = r * math::Sqrt(m_gm / (s * math::Sqrt(s)));

    /* The random velocities the brief asked for. Perturbing the tangential speed alone would leave every
     * particle starting at an apsis of its own ellipse, which shows up as a visible ring at the seed radius, so
     * there is a radial kick too - that spreads the starting true anomalies out. */
    const double tangentialScale = 1. + m_speedJitter * (2. * unit(gen) - 1.);
    const double radialScale = 0.5 * m_speedJitter * (2. * unit(gen) - 1.);
    const double direction = unit(gen) < 0.5 ? -1. : 1.;

    m_pos[i] = Center() + radial * r;
    m_vel[i] = tangent * (vc * tangentialScale * direction) + radial * (vc * radialScale);
  }

  m_accumulator = 0.;
}

void PointCloud::Step(double _h) noexcept {
  const size_t n = m_pos.size();
  Vec3d *pos = m_pos.data();
  Vec3d *vel = m_vel.data();

  const double cx = Center().x, cy = Center().y;
  const double eps2 = m_softening * m_softening;

  for (size_t i = 0; i < n; ++i) {
    const double dx = cx - pos[i].x;
    const double dy = cy - pos[i].y;
    const double r2 = dx * dx + dy * dy + eps2;
    const double invR = 1. / math::Sqrt(r2);
    const double a = m_gm * invR * invR * invR;   //!< GM / (r² + ε²)^{3/2}

    /* Semi-implicit Euler: velocity advances first, then position integrates against the *new* velocity. One
     * character of difference from the explicit form, and it is the difference between orbits that hold their
     * shape indefinitely and orbits that spiral outward - the semi-implicit form is symplectic, so its energy
     * error stays bounded instead of accumulating. This is also what Core's `Motion` behavior does. */
    vel[i].x += dx * a * _h;
    vel[i].y += dy * a * _h;
    pos[i].x += vel[i].x * _h;
    pos[i].y += vel[i].y * _h;
  }
}

void PointCloud::FrameUpdate() {
  if (InputSystem::GetKey(KeyCode::ESCAPE)) {
    m_object->GetScene().GetApplication().Quit();
    return;
  }
  if (InputSystem::GetKeyDown(KeyCode::R)) Reseed();

  /* The engine declares a FixedUpdate hook but never calls it, so the fixed-step loop lives here. Stepping at a
   * fixed h rather than at Time::deltaTime matters: a step size that wobbles with the frame rate makes the
   * integration frame-rate dependent, and the cloud would evolve differently on a different machine. */
  m_accumulator += math::Min(Time::deltaTime, maxFrameSeconds);

  int taken = 0;
  while (m_accumulator >= m_step && taken < maxSubstepsPerFrame) {
    Step(m_step);
    m_accumulator -= m_step;
    ++taken;
  }
  if (taken == maxSubstepsPerFrame) m_accumulator = 0.;

  Draw();
}

void PointCloud::Draw() noexcept {
  if (!m_renderer) return;
  auto *renderer = static_cast<SDL_Renderer *>(m_renderer);

  SDL_SetRenderDrawColor(renderer, 255, 226, 150, SDL_ALPHA_OPAQUE);
  FillDisc(renderer, static_cast<float>(Center().x), static_cast<float>(Center().y), centralRadius);

  /* Collect the on-screen particles into one contiguous run and draw them in a single call, rather than one SDL
   * call per particle. */
  const size_t n = m_pos.size();
  const Vec3d *pos = m_pos.data();
  ScreenPoint *screen = m_screen.data();
  size_t drawn = 0;

  for (size_t i = 0; i < n; ++i) {
    if (pos[i].x < 0. || pos[i].y < 0. || pos[i].x >= m_width || pos[i].y >= m_height) continue;
    screen[drawn].x = static_cast<float>(pos[i].x);
    screen[drawn].y = static_cast<float>(pos[i].y);
    ++drawn;
  }

  if (drawn == 0) return;
  SDL_SetRenderDrawColor(renderer, 200, 220, 255, SDL_ALPHA_OPAQUE);
  SDL_RenderPoints(renderer, reinterpret_cast<const SDL_FPoint *>(screen), static_cast<int>(drawn));
}
