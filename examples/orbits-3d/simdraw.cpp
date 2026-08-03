#line 2 "examples/orbits/simdraw.cpp"
/**

  @file       simdraw.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "simdraw.h"

#include <SDL3/SDL.h>
#include <cstddef>

using namespace ROSE;

/* ScreenPoint exists so that orbitcamera.h does not have to include SDL. It only earns its keep if an array of
 * them can be handed straight to SDL without a per-point copy, which is what these checks are guarding: same size,
 * same alignment, same member offsets, same member types. If SDL_FPoint ever changes, this fails to compile here
 * rather than silently drawing garbage. */
static_assert(sizeof(ScreenPoint) == sizeof(SDL_FPoint));
static_assert(alignof(ScreenPoint) == alignof(SDL_FPoint));
static_assert(offsetof(ScreenPoint, x) == offsetof(SDL_FPoint, x));
static_assert(offsetof(ScreenPoint, y) == offsetof(SDL_FPoint, y));
static_assert(std::is_same_v<decltype(ScreenPoint::x), decltype(SDL_FPoint::x)>);

namespace {
  constexpr size_t pathRunCapacity { 512 };

  /*! Stroke a run of consecutive on-screen points and reset it. */
  void FlushRun(SDL_Renderer *_renderer, ScreenPoint *_run, size_t &_count) noexcept {
    if (_count >= 2) SDL_RenderLines(_renderer, reinterpret_cast<const SDL_FPoint *>(_run), static_cast<int>(_count));
    _count = 0;
  }
} // namespace

void orb::SetColor(void *_renderer, uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) noexcept {
  if (!_renderer) return;
  SDL_SetRenderDrawColor(static_cast<SDL_Renderer *>(_renderer), _r, _g, _b, _a);
}

void orb::DrawPoints(void *_renderer, const ScreenPoint *_points, size_t _count) noexcept {
  if (!_renderer || !_points || _count == 0) return;
  SDL_RenderPoints(static_cast<SDL_Renderer *>(_renderer), reinterpret_cast<const SDL_FPoint *>(_points),
                   static_cast<int>(_count));
}

void orb::FillDisc(void *_renderer, float _cx, float _cy, float _radius) noexcept {
  if (!_renderer || _radius <= 0.f) return;
  auto *renderer = static_cast<SDL_Renderer *>(_renderer);

  if (_radius < 1.f) {
    SDL_RenderPoint(renderer, _cx, _cy);
    return;
  }

  const int rad = static_cast<int>(_radius);
  const float r2 = _radius * _radius;
  for (int dy = -rad; dy <= rad; ++dy) {
    const float y = static_cast<float>(dy);
    const float halfSpan = math::Sqrt(r2 - y * y);
    SDL_RenderLine(renderer, _cx - halfSpan, _cy + y, _cx + halfSpan, _cy + y);
  }
}

void orb::DrawProjectedPath(void *_renderer, const OrbitCamera &_camera, const Vec3d *_points,
                            size_t _count) noexcept {
  if (!_renderer || !_points || _count < 2) return;
  auto *renderer = static_cast<SDL_Renderer *>(_renderer);

  ScreenPoint run[pathRunCapacity];
  size_t runCount { 0 };

  for (size_t i = 0; i < _count; ++i) {
    const Projection p = _camera.Project(_points[i]);
    if (!p.visible) {
      FlushRun(renderer, run, runCount);
      continue;
    }
    if (runCount == pathRunCapacity) {
      /* Keep the last point so the next batch joins seamlessly onto this one. */
      const ScreenPoint tail = run[runCount - 1];
      FlushRun(renderer, run, runCount);
      run[runCount++] = tail;
    }
    run[runCount++] = p.point;
  }
  FlushRun(renderer, run, runCount);
}
