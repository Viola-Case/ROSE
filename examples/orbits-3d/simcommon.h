#line 2 "examples/orbits/simcommon.h"
/**

  @file       simcommon.h
  @brief      Small shared helpers for the orbits example.
  @details    Vector maths the engine's `math` layer does not provide yet, plus a scene-wide behavior lookup.
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/ROSE.h>

using namespace ROSE;

/*!
 * Everything in here is example-local scaffolding rather than engine API, so it lives in its own namespace instead
 * of leaking into the global one the way `using namespace ROSE` does.
 */
namespace orb {

  /*!
   * `math::PI` is initialised from a float literal, so it only carries float precision (~8.7e-8 relative error;
   * see docs/internal/known-issues.md #8). That error is invisible when it only decides where a pixel lands, but
   * this example integrates orbits over many thousands of steps and reports energy drift down to 1e-12 - a
   * float-precision π would show up as a floor on the measurement. Use a real double here.
   */
  constexpr double pi { 3.14159265358979323846 };

  constexpr double twoPi { 2. * pi };

  [[nodiscard]] constexpr double LengthSq(const Vec3d &_v) noexcept { return _v.dot(_v); }

  [[nodiscard]] inline double Length(const Vec3d &_v) noexcept { return math::Sqrt(_v.dot(_v)); }

  [[nodiscard]] constexpr Vec3d Negated(const Vec3d &_v) noexcept { return { -_v.x, -_v.y, -_v.z }; }

  /*! Unit vector along `_v`, or the zero vector if `_v` is degenerate. Never returns NaN. */
  [[nodiscard]] inline Vec3d Unit(const Vec3d &_v) noexcept {
    const double n = Length(_v);
    return n > 0. ? _v * (1. / n) : Vec3d { 0., 0., 0. };
  }

  /*! Any unit vector perpendicular to `_v`. `_v` is assumed unit-length. */
  [[nodiscard]] inline Vec3d AnyPerpendicular(const Vec3d &_v) noexcept {
    /* Cross with whichever axis `_v` is least aligned to, so the cross product never collapses. */
    const Vec3d axis = _v.y * _v.y < 0.9 ? Vec3d { 0., 1., 0. } : Vec3d { 1., 0., 0. };
    return Unit(_v.cross(axis));
  }

  [[nodiscard]] constexpr double Clamp(double _v, double _lo, double _hi) noexcept {
    return _v < _lo ? _lo : (_v > _hi ? _hi : _v);
  }

  /*!
   * First behavior of type `B` anywhere in `_scene`, or nullptr. The scene is a flat unordered map, so "first" is
   * whichever the hash order hands over - fine when there is exactly one of a kind, which is how this example is
   * laid out. Call it from `OnStart()`, never `OnCreate()`: the neighbours do not exist yet in phase one.
   */
  template <class B>
  [[nodiscard]] B *FindInScene(Scene &_scene) noexcept {
    B *found { nullptr };
    _scene.ForEachObject([&](Object &o) {
      if (found) return;
      if (B *b = o.FindBehavior<B>()) found = b;
    });
    return found;
  }

} // namespace orb
