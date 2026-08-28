/**

    @file      trailrenderer.h
    @brief     Batched ribbon renderer for a whole cloud of trails.
    @details   ~
    @author    Viola Case
    @date      27.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <SDL3/SDL_render.h>

#include "trail.h"

namespace Orbits {
  //! Straight (non-premultiplied) 0..1 RGBA, matching `SDL_FColor`.
  struct TrailStyle {
    SDL_FColor head { 1.0f, 1.0f, 1.0f, 1.0f };  //!< at the body
    SDL_FColor tail { 0.25f, 0.5f, 1.0f, 0.0f }; //!< at the far end; alpha 0 fades the trail out
    float headWidth { 2.0f };                    //!< ribbon width in pixels at the body
    float tailWidth { 0.5f };                    //!< ribbon width in pixels at the far end
  };

  /*!
   * Draws every trail in the cloud in a single `SDL_RenderGeometry` call.
   *
   * The obvious shape - one `SDL_RenderPoints`/`SDL_RenderLines` per trail - costs a draw
   * call per body, and SDL cannot merge them because a line strip would join trails that are
   * nowhere near each other. Expanding each trail into a triangle ribbon instead makes the
   * whole cloud one indexed batch whose cost is independent of how many bodies there are,
   * and buys per-vertex color, so the trail can fade along its length for free.
   *
   * The vertex and index buffers are members rather than locals so they keep their capacity
   * between frames; the indices only depend on the buffer's shape, so they are rebuilt only
   * while the ring is still filling and left alone after that.
   */
  class TrailRenderer {
  public:
    void Draw(SDL_Renderer *renderer, const TrailBuffer &trails, const TrailStyle &style);

    [[nodiscard]] size_t VertexCount() const noexcept { return m_vertices.size(); }
    [[nodiscard]] size_t TriangleCount() const noexcept { return m_indices.size() / 3; }

  private:
    void buildIndices(uint32_t trails, uint32_t samples);

    List<SDL_Vertex> m_vertices;
    List<int> m_indices;
    uint32_t m_indexedTrails { 0 };  //!< shape the current index buffer was built for
    uint32_t m_indexedSamples { 0 };
  };
} // namespace Orbits
