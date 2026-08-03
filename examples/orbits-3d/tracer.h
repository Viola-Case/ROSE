#line 2 "examples/orbits/tracer.h"
/**

  @file       tracer.h
  @brief      One body on the same central force, integrated by Core's `Motion` instead of by the cloud.
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/ROSE.h>

using namespace ROSE;

class CentralBody;
class OrbitCamera;

/*!
 * The control group. This behavior owns no state vector of its own: it sits on an object that also carries a
 * `Motion`, writes the gravitational acceleration into it every frame, and lets the engine integrate.
 *
 * Two things differ from the cloud, and both are visible on the HUD:
 *
 * 1. `Motion` integrates with **semi-implicit Euler** at whatever `Time::deltaTime` happens to be, so its step
 *    size is the frame time. The cloud runs a fixed step. A variable step destroys the exact energy conservation
 *    a symplectic integrator would otherwise give you - the method is still symplectic for each individual step,
 *    but the map changes every frame, so the errors no longer cancel and the energy random-walks.
 * 2. Behavior update order within an object is unspecified (`m_behaviors` is a hash map), so the acceleration
 *    `Motion` reads may be the one written on the *previous* frame. At 60 fps that is a ~16 ms lag on a slowly
 *    varying field, which is small but not nothing - it is part of why the tracer drifts faster than the cloud.
 *
 * Neither is a defect in `Motion`; they are what you get from a per-object gameplay integrator, which is what it
 * is for. The comparison is the point.
 */
class Tracer : public Behavior {
public:
  static constexpr UUID typeID = "30131a27e9b7af29-9c7ef820796596b1"_uuid;
  static constexpr UUID TypeID() noexcept { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }

  /*! Put the tracer back on its seed orbit and restart the energy baseline. */
  void Reset() noexcept;

  [[nodiscard]] bool IsLive() const noexcept { return m_motion != nullptr && m_body != nullptr; }
  [[nodiscard]] double GetEnergy() const noexcept { return m_energy; }
  [[nodiscard]] double GetEnergy0() const noexcept { return m_energy0; }
  [[nodiscard]] double GetEnergyDrift() const noexcept;
  [[nodiscard]] double GetRadius() const noexcept { return m_radius; }

protected:
  /*!
   * Unpack takes JSON object
   * {
   *    "radius": 120.0,       // seed radius, world units
   *    "inclination": 0.4,    // radians, tilt of the orbital plane away from xz
   *    "trailLength": 900     // path samples retained; 0 disables the trail
   * }
   */
  void Unpack(const ParamView &_view) override;
  void OnStart() override;
  void FrameUpdate() override;

  double m_radius { 120. };
  double m_inclination { 0.4 };
  size_t m_trailLength { 900 };

  /* Ring buffer of past positions. Stored uncompacted with a head index so appending is O(1); `Rebuild` in
   * simdraw wants a contiguous run, so the draw call unrolls it into m_trailOrdered. */
  List<Vec3d> m_trail {};
  List<Vec3d> m_trailOrdered {};
  size_t m_trailHead { 0 };
  size_t m_trailFilled { 0 };

  double m_energy { 0. };
  double m_energy0 { 0. };

  Motion *m_motion { nullptr };
  CentralBody *m_body { nullptr };
  OrbitCamera *m_camera { nullptr };
  void *m_renderer { nullptr };
};
