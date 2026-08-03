#line 2 "examples/orbits/centralbody.h"
/**

  @file       centralbody.h
  @brief      The attractor: the gravitational field everything else in the scene falls through.
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/ROSE.h>

using namespace ROSE;

class OrbitCamera;

/*!
 * A fixed point mass at its own object's transform, using a Plummer-softened potential
 *
 * \f[ \Phi(r) = \frac{-GM}{\sqrt{r^2 + \varepsilon^2}} \qquad
 *     \vec a(\vec r) = -GM \frac{\vec r}{\left(r^2 + \varepsilon^2\right)^{3/2}} \f]
 *
 * The softening length \f$\varepsilon\f$ is not cosmetic. An unsoftened \f$1/r^2\f$ force is singular at the
 * origin, and any particle whose random initial velocity puts it on a nearly radial orbit will eventually pass
 * close enough that the acceleration over one timestep exceeds anything the integrator can represent - it gets
 * slingshotted to infinity and the energy books never balance again. Softening caps the force at
 * \f$GM/\varepsilon^2\f$ and, crucially, the acceleration above is *exactly* the gradient of the potential above,
 * so the softened system is still conservative and energy drift still measures integrator error and nothing else.
 *
 * The mass is treated as fixed rather than as a body that also moves: the cloud is made of test particles that
 * feel the centre but neither it nor each other. That keeps the force evaluation \f$O(N)\f$ instead of
 * \f$O(N^2)\f$ and keeps the conserved quantities analytic.
 */
class CentralBody : public Behavior {
public:
  static constexpr UUID typeID = "4720cd2d415ed83e-e57239b840659bb0"_uuid;
  static constexpr UUID TypeID() noexcept { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }

  [[nodiscard]] double GetGM() const noexcept { return m_gm; }
  [[nodiscard]] double GetSoftening() const noexcept { return m_softening; }
  [[nodiscard]] const Vec3d &GetCenter() const noexcept { return m_object->transform.position; }

  void SetGM(double _gm) noexcept { m_gm = _gm > 0. ? _gm : m_gm; }
  void SetSoftening(double _s) noexcept { m_softening = _s >= 0. ? _s : m_softening; }

  /*! Acceleration a unit mass at `_at` feels. */
  [[nodiscard]] Vec3d AccelerationAt(const Vec3d &_at) const noexcept;

  /*! Potential energy per unit mass at `_at`; negative everywhere, approaching zero at infinity. */
  [[nodiscard]] double PotentialAt(const Vec3d &_at) const noexcept;

  /*! Speed of a circular orbit of radius `_r`, accounting for the softening. */
  [[nodiscard]] double CircularSpeed(double _r) const noexcept;

protected:
  /*!
   * Unpack takes JSON object
   * {
   *    "gm": 60000.0,        // GM, the standard gravitational parameter; the only mass term that matters
   *    "softening": 3.0,     // ε, in world units
   *    "radius": 7.0,        // drawn radius, world units - purely cosmetic
   *    "ringRadius": 0.0     // if > 0, draw a reference circle of this radius in the xz plane
   * }
   */
  void Unpack(const ParamView &_view) override;
  void OnStart() override;
  void FrameUpdate() override;

  double m_gm { 60000. };
  double m_softening { 3. };
  double m_radius { 7. };
  double m_ringRadius { 0. };

  void *m_renderer { nullptr };
  OrbitCamera *m_camera { nullptr };
};
