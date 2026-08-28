/**

    @file      trailrenderer.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      27.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include "trailrenderer.h"

#include <ROSE/Core/math.h>

static_assert(sizeof(Orbits::Point) == sizeof(SDL_FPoint) && alignof(Orbits::Point) == alignof(SDL_FPoint),
              "Point is handed to SDL as an SDL_FPoint array; the two must stay layout-compatible.");

namespace Orbits {
  namespace {
    //! Below this a segment has no direction to take a normal from, so it contributes nothing.
    constexpr float DEGENERATE = 1e-6f;

    constexpr SDL_FColor Lerp(const SDL_FColor &from, const SDL_FColor &to, float t) noexcept {
      return { from.r + (to.r - from.r) * t, from.g + (to.g - from.g) * t, from.b + (to.b - from.b) * t,
               from.a + (to.a - from.a) * t };
    }
  } // namespace

  void TrailRenderer::buildIndices(uint32_t trails, uint32_t samples) {
    /* Which vertices form which triangles depends only on the buffer's shape, and that stops
     * changing once the ring is full - after which this is 60 skipped rebuilds a second. */
    if (trails == m_indexedTrails && samples == m_indexedSamples) return;

    const uint32_t segments = samples - 1;
    m_indices.clear();
    m_indices.reserve(static_cast<size_t>(trails) * segments * 6);
    for (uint32_t t = 0; t < trails; ++t) {
      const int base = static_cast<int>(t * samples * 2);
      for (uint32_t s = 0; s < segments; ++s) {
        // Two triangles across the ribbon: (left, right, next left) and (right, next right, next left).
        const int v = base + static_cast<int>(s) * 2;
        m_indices.push_back(v);
        m_indices.push_back(v + 1);
        m_indices.push_back(v + 2);
        m_indices.push_back(v + 1);
        m_indices.push_back(v + 3);
        m_indices.push_back(v + 2);
      }
    }

    m_indexedTrails = trails;
    m_indexedSamples = samples;
  }

  void TrailRenderer::Draw(SDL_Renderer *renderer, const TrailBuffer &trails, const TrailStyle &style) {
    const uint32_t count = trails.TrailCount();
    const uint32_t samples = trails.Filled();
    if (!renderer || count == 0 || samples < 2) return; // nothing to connect on the first frame

    buildIndices(count, samples);

    m_vertices.clear();
    m_vertices.reserve(static_cast<size_t>(count) * samples * 2);

    const float span = static_cast<float>(samples - 1);
    for (uint32_t t = 0; t < count; ++t) {
      for (uint32_t s = 0; s < samples; ++s) {
        const Point here = trails.At(t, s);
        /* Central difference along the trail, clamped at both ends, so the ribbon's normal
         * follows the curve instead of kinking at every sample. Adjacent segments share these
         * two vertices, which is also what halves the vertex count against emitting quads. */
        const Point prev = trails.At(t, s ? s - 1 : s);
        const Point next = trails.At(t, s + 1 < samples ? s + 1 : s);

        float nx = -(next.y - prev.y);
        float ny = next.x - prev.x;
        if (const float len = ROSE::math::Sqrt(nx * nx + ny * ny); len > DEGENERATE) {
          nx /= len;
          ny /= len;
        } else {
          nx = ny = 0.0f; // a body that has not moved collapses to a zero-area triangle
        }

        const float along = static_cast<float>(s) / span; // 0 at the tail, 1 at the body
        const float half = (style.tailWidth + (style.headWidth - style.tailWidth) * along) * 0.5f;
        const SDL_FColor color = Lerp(style.tail, style.head, along);

        m_vertices.emplace_back(SDL_Vertex { { here.x + nx * half, here.y + ny * half }, color, {} });
        m_vertices.emplace_back(SDL_Vertex { { here.x - nx * half, here.y - ny * half }, color, {} });
      }
    }

    // The fade lives in the vertex alpha, so it only shows up with blending on.
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(renderer, nullptr, m_vertices.data(), static_cast<int>(m_vertices.size()), m_indices.data(),
                       static_cast<int>(m_indices.size()));
  }
} // namespace Orbits
