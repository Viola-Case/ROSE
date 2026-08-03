#line 2 "examples/orbits/orbitcamera.h"
/**

  @file       orbitcamera.h
  @brief      Turntable camera and the pinhole projection every renderer in this example shares.
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/ROSE.h>

using namespace ROSE;

/*!
 * A point in screen space. The layout is deliberately `{ float x; float y; }` so that a whole array of them can be
 * handed to `SDL_RenderPoints` without a copy; `pointcloud.cpp` static_asserts that against `SDL_FPoint`.
 */
struct ScreenPoint {
  float x { 0.f };
  float y { 0.f };
};

struct Projection {
  ScreenPoint point {};
  double depth { 0. };      //!< distance along the view axis in world units; behind the camera when `visible` is false
  bool visible { false };   //!< false if the point is behind the near plane
};

/*!
 * Orbits the origin of its own object's transform at a fixed distance, driven from the keyboard. `Core/camera.h`
 * exists but nothing in the renderer consumes it yet, so this example carries its own and does the projection by
 * hand.
 *
 * @note Behavior update order within a frame is unspecified (hash-map order), so a renderer that projects through
 * this camera may be using the basis from the previous frame. At interactive turn rates that is invisible, and
 * `OnStart` builds a valid basis before the first `FrameUpdate` so nothing ever projects through a zero matrix.
 */
class OrbitCamera : public Behavior {
public:
  static constexpr UUID typeID = "afff5a43bcae9747-303b41fa30659b14"_uuid;
  static constexpr UUID TypeID() noexcept { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }

  /*! Project a world-space point onto the viewport. */
  [[nodiscard]] Projection Project(const Vec3d &_world) const noexcept;

  /*! Screen pixels spanned by one world unit at `_depth`. Renderers size their discs with this. */
  [[nodiscard]] double PixelsPerUnit(double _depth) const noexcept;

  [[nodiscard]] const Vec3d &GetEye() const noexcept { return m_eye; }
  [[nodiscard]] double GetDistance() const noexcept { return m_distance; }
  [[nodiscard]] double GetViewWidth() const noexcept { return m_halfW * 2.; }
  [[nodiscard]] double GetViewHeight() const noexcept { return m_halfH * 2.; }
  [[nodiscard]] bool IsSpinning() const noexcept { return m_spin; }

  void ToggleSpin() noexcept { m_spin = !m_spin; }
  void ResetView() noexcept;

protected:
  /*!
   * Unpack takes JSON object
   * {
   *    "yaw": 0.6,             // radians, around the world +y axis
   *    "pitch": 0.35,          // radians, positive looks down at the scene
   *    "distance": 320.0,      // world units from the target
   *    "fov": 55.0,            // vertical field of view, degrees
   *    "spin": false,          // start with the turntable running
   *    "spinRate": 0.15        // radians per second when spinning
   * }
   *
   * @note Every one of these is read through `ParamView::GetDouble`, which only accepts JSON *floats* - a bare
   * `320` silently falls back to the default while `320.0` works. Keep the decimal points in orbits.json.
   */
  void Unpack(const ParamView &_view) override;
  void OnStart() override;
  void FrameUpdate() override;

  /*! Recompute the eye position and the orthonormal view basis from yaw/pitch/distance. */
  void Rebuild() noexcept;

  double m_yaw { 0.6 };
  double m_pitch { 0.35 };
  double m_distance { 320. };
  double m_fov { 55. };
  double m_spinRate { 0.15 };
  bool m_spin { false };

  double m_yaw0 { 0.6 };       //!< the unpacked values, so HOME can put the view back
  double m_pitch0 { 0.35 };
  double m_distance0 { 320. };

  Vec3d m_eye {};
  Vec3d m_forward {};
  Vec3d m_right {};
  Vec3d m_up {};

  double m_halfW { 400. };
  double m_halfH { 300. };
  double m_focalPx { 0. };     //!< focal length in pixels, derived from the vertical fov and the viewport height
};
