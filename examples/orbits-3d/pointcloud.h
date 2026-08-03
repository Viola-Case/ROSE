#line 2 "examples/orbits/pointcloud.h"
/**

  @file       pointcloud.h
  @brief      N independent test particles on random orbits about a CentralBody, integrated four different ways.
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include "orbitcamera.h"

#include <ROSE/ROSE.h>

using namespace ROSE;

class CentralBody;

/*!
 * The time integrators the cloud can be stepped with, in ascending order of how well they behave. Switchable at
 * runtime because the whole point of the example is watching the same initial conditions diverge under each.
 */
enum class Integrator : int {
  ForwardEuler = 0,     //!< explicit Euler; first order, not symplectic. Pumps energy in and spirals outward.
  SemiImplicitEuler,    //!< velocity first, then position against the new velocity. First order but symplectic.
  VelocityVerlet,       //!< kick-drift-kick. Second order, symplectic, time reversible. The sensible default.
  RungeKutta4,          //!< classical RK4. Fourth order but *not* symplectic: accurate, and still drifts secularly.
  Count
};

[[nodiscard]] const char *IntegratorName(Integrator _i) noexcept;
[[nodiscard]] const char *IntegratorNote(Integrator _i) noexcept;

/*! Everything the HUD reads. Recomputed once per frame, not once per substep. */
struct SimStats {
  double time { 0. };                //!< simulated seconds since the last reseed
  double kinetic { 0. };
  double potential { 0. };
  double energy { 0. };              //!< kinetic + potential, summed over the cloud
  double energy0 { 0. };             //!< the same, measured immediately after seeding
  double angularMomentum { 0. };     //!< |ΣL|, magnitude of the total angular momentum
  double angularMomentum0 { 0. };
  size_t steps { 0 };                //!< integrator steps since the last reseed
  int substeps { 0 };                //!< integrator steps taken during the last frame
  size_t drawn { 0 };                //!< particles that survived frustum culling last frame
  double stepMilliseconds { 0. };    //!< wall time spent integrating during the last frame
};

/*!
 * A cloud of unit-mass test particles orbiting a `CentralBody`.
 *
 * State is stored structure-of-arrays (`m_pos`, `m_vel`, `m_acc`) rather than as one `Object` per particle. That
 * is a deliberate departure from the engine's composition model: an `Object` carrying a `Motion` is the idiomatic
 * way to move one thing, but N of them means N hash-map entries, N virtual `FrameUpdate` calls and a scattered
 * heap walk per step, and it fixes the integrator to whatever `Motion` implements. The `Tracer` behavior next
 * door does it the idiomatic way with a single body so the two can be compared directly.
 *
 * The particles do not interact with each other - only with the fixed central mass - so a step is O(N).
 */
class PointCloud : public Behavior {
public:
  static constexpr UUID typeID = "f69e87e51985f92b-cd8aa43477659a22"_uuid;
  static constexpr UUID TypeID() noexcept { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }

  /*! Rebuild the whole cloud from the current seed and distribution parameters, and reset the conserved baselines. */
  void Reseed() noexcept;

  [[nodiscard]] size_t GetCount() const noexcept { return m_pos.size(); }
  void SetCount(size_t _n) noexcept;

  [[nodiscard]] uint32_t GetSeed() const noexcept { return m_seed; }
  void SetSeed(uint32_t _seed) noexcept;

  [[nodiscard]] Integrator GetIntegrator() const noexcept { return m_integrator; }
  void SetIntegrator(Integrator _i) noexcept;

  [[nodiscard]] double GetStep() const noexcept { return m_step; }
  void SetStep(double _h) noexcept;

  [[nodiscard]] double GetTimeScale() const noexcept { return m_timeScale; }
  void SetTimeScale(double _s) noexcept;

  [[nodiscard]] bool IsPaused() const noexcept { return m_paused; }
  void SetPaused(bool _p) noexcept { m_paused = _p; }
  void TogglePaused() noexcept { m_paused = !m_paused; }

  [[nodiscard]] double GetSpeedJitter() const noexcept { return m_speedJitter; }
  void SetSpeedJitter(double _j) noexcept;

  [[nodiscard]] double GetInclinationSpread() const noexcept { return m_inclinationSpread; }
  void SetInclinationSpread(double _s) noexcept;

  [[nodiscard]] const SimStats &GetStats() const noexcept { return m_stats; }

  /*! Relative energy drift since seeding. This is the number the whole example exists to show. */
  [[nodiscard]] double GetEnergyDrift() const noexcept;
  [[nodiscard]] double GetAngularMomentumDrift() const noexcept;

protected:
  /*!
   * Unpack takes JSON object
   * {
   *    "count": 6000,              // particles
   *    "seed": 24601,              // PRNG seed; the same seed always builds the same cloud
   *    "radiusMin": 45.0,          // inner edge of the seeded shell, world units
   *    "radiusMax": 180.0,         // outer edge
   *    "speedJitter": 0.22,        // 0 = perfectly circular orbits, 1 = wildly eccentric (some unbound)
   *    "inclinationSpread": 0.35,  // 0 = a flat disc, 1 = an isotropic spherical swarm
   *    "step": 0.004,              // integrator step h, simulated seconds
   *    "timeScale": 1.0,           // simulated seconds per wall second
   *    "integrator": 2             // index into Integrator
   * }
   *
   * @note `count`, `seed` and `integrator` go through `GetInt`, which accepts JSON integers. Everything else goes
   * through `GetDouble`, which does *not* - it gates on `is_number_float()`, so `"step": 1` silently falls back to
   * the default while `"step": 1.0` works. Keep the decimal points.
   */
  void Unpack(const ParamView &_view) override;
  void OnStart() override;
  void FrameUpdate() override;

  /*! Softened central acceleration for every particle in `_pos`, written to `_acc`. The inner loop. */
  void Accelerate(const List<Vec3d> &_pos, List<Vec3d> &_acc) const noexcept;

  /*! Advance the whole cloud by exactly `_h` using the selected integrator. */
  void Step(double _h) noexcept;

  /*! Recompute the conserved quantities. O(N), so it runs once per frame rather than once per step. */
  void Measure() noexcept;

  void Draw() noexcept;
  void Allocate(size_t _n) noexcept;
  void BuildPalette() noexcept;

  /* ---- simulation state ---- */
  List<Vec3d> m_pos {};
  List<Vec3d> m_vel {};
  List<Vec3d> m_acc {};

  /* RK4 scratch. Four accelerations and four velocities per particle, plus one position buffer to evaluate the
   * stages at. Allocated with everything else so a step never touches the allocator. */
  List<Vec3d> m_rkPos {};
  List<Vec3d> m_rkA[4] {};
  List<Vec3d> m_rkV[4] {};

  Integrator m_integrator { Integrator::VelocityVerlet };
  double m_step { 0.004 };
  double m_timeScale { 1. };
  double m_accumulator { 0. };
  bool m_paused { false };

  /* ---- distribution parameters ---- */
  size_t m_count { 6000 };
  uint32_t m_seed { 24601 };
  double m_radiusMin { 45. };
  double m_radiusMax { 180. };
  double m_speedJitter { 0.22 };
  double m_inclinationSpread { 0.35 };

  /* ---- cached from the CentralBody, refreshed every frame so moving it works ---- */
  CentralBody *m_body { nullptr };
  Vec3d m_center {};
  double m_gm { 1. };
  double m_softening { 0. };

  SimStats m_stats {};

  /* ---- rendering ---- */
  static constexpr size_t colorBins { 16 };   //!< speed-relative-to-circular bins
  static constexpr size_t shadeBins { 6 };    //!< depth-attenuation bins
  static constexpr size_t bucketCount { colorBins * shadeBins };

  /* Points are bucketed by colour so the frame costs `bucketCount` SDL calls instead of one per particle. */
  List<ScreenPoint> m_buckets[bucketCount] {};
  uint8_t m_bucketRGB[bucketCount][3] {};

  void *m_renderer { nullptr };
  OrbitCamera *m_camera { nullptr };
};
