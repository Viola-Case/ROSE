#line 2 "examples/orbits/pointcloud.h"
/**

  @file       pointcloud.h
  @brief      A cloud of test particles on random orbits about a fixed central mass.
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/ROSE.h>

using namespace ROSE;

/*!
 * A screen-space point. Laid out as two floats on purpose so an array of them can be handed straight to
 * `SDL_RenderPoints` without a copy; pointcloud.cpp static_asserts that against `SDL_FPoint`. Declaring it this
 * way keeps SDL out of the header, the way `paddle.h` forward-declares `SDL_Renderer`.
 */
struct ScreenPoint {
  float x { 0.f };
  float y { 0.f };
};

/*!
 * N unit-mass particles falling around a fixed point mass at the centre of the window.
 *
 * The particles feel the centre but not each other, so a step is O(N) rather than O(N²) and the whole cloud fits
 * in three flat arrays. Working directly in window pixels keeps it two-dimensional and means no camera or
 * projection is involved - what you see is the state vector.
 *
 * The force is Plummer-softened,
 *
 * \f[ \vec a(\vec r) = -GM \frac{\vec r}{\left(r^2 + \varepsilon^2\right)^{3/2}} \f]
 *
 * which is not cosmetic. A bare \f$1/r^2\f$ force is singular at the origin, and a particle whose random initial
 * velocity puts it on a nearly radial orbit eventually passes close enough that the acceleration over one
 * timestep is enormous - it gets slingshotted off the screen and never comes back. Softening caps the force at
 * \f$GM/\varepsilon^2\f$.
 */
class PointCloud : public Behavior {
public:
  static constexpr UUID typeID = "f69e87e51985f92b-cd8aa43477659a22"_uuid;
  static constexpr UUID TypeID() noexcept { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }

  /*! Rebuild the whole cloud from the current seed. */
  void Reseed() noexcept;

protected:
  /*!
   * Unpack takes JSON object
   * {
   *    "count": 4000,          // particles
   *    "seed": 24601,          // PRNG seed; the same seed always builds the same cloud
   *    "gm": 5000000.0,        // GM, the standard gravitational parameter
   *    "softening": 4.0,       // ε, in pixels
   *    "radiusMin": 60.0,      // inner edge of the seeded annulus, pixels
   *    "radiusMax": 320.0,     // outer edge
   *    "speedJitter": 0.25,    // 0 = perfectly circular orbits, 1 = wildly eccentric
   *    "step": 0.002           // integrator step h, simulated seconds
   * }
   *
   * @note `count` and `seed` go through `GetInt`, which accepts JSON integers. Everything else goes through
   * `GetDouble`, which does *not* - it gates on `is_number_float()`, so `"softening": 4` silently falls back to
   * the default while `"softening": 4.0` works. Keep the decimal points.
   */
  void Unpack(const ParamView &_view) override;
  void OnStart() override;
  void FrameUpdate() override;

  /*! Advance every particle by exactly `_h` with semi-implicit Euler. */
  void Step(double _h) noexcept;

  void Draw() noexcept;

  List<Vec3d> m_pos {};
  List<Vec3d> m_vel {};
  List<ScreenPoint> m_screen {};

  size_t m_count { 4000 };
  uint32_t m_seed { 24601 };
  double m_gm { 5000000. };
  double m_softening { 4. };
  double m_radiusMin { 60. };
  double m_radiusMax { 320. };
  double m_speedJitter { 0.25 };
  double m_step { 0.002 };
  double m_accumulator { 0. };

  /*! The attractor sits at the owning object's transform, which `OnStart` parks at the middle of the window. */
  [[nodiscard]] const Vec3d &Center() const noexcept { return m_object->transform.position; }

  void *m_renderer { nullptr };
  int m_width { 0 };
  int m_height { 0 };
};
