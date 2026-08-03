#line 2 "examples/orbits/simdraw.h"
/**

  @file       simdraw.h
  @brief      The handful of SDL drawing primitives this example needs, with SDL kept out of the headers.
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include "orbitcamera.h"

#include <ROSE/ROSE.h>

using namespace ROSE;

namespace orb {

  void SetColor(void *_renderer, uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) noexcept;

  /*! Batch-draw already-projected points in the current draw colour. One SDL call, whatever `_count` is. */
  void DrawPoints(void *_renderer, const ScreenPoint *_points, size_t _count) noexcept;

  /*! Axis-aligned filled disc, drawn as horizontal spans. SDL has no circle primitive. */
  void FillDisc(void *_renderer, float _cx, float _cy, float _radius) noexcept;

  /*!
   * Project `_count` world-space points and stroke them as a polyline. The line is broken wherever a point falls
   * behind the near plane rather than drawn across the screen to a bogus position, so a path that leaves the
   * frustum comes back as separate runs.
   */
  void DrawProjectedPath(void *_renderer, const OrbitCamera &_camera, const Vec3d *_points, size_t _count) noexcept;

} // namespace orb
