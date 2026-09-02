/**

    @file      trailrenderer.h
    @brief     Batched ribbon geometry for a whole cloud of trails.
    @details   ~
    @author    Viola Case
    @date      27.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <ROSE/Core/gfx.h>

#include "trail.h"

namespace Orbits {
  using ROSE::DrawVertex;
  using ROSE::Vec4f;

  //! Straight (non-premultiplied) 0..1 RGBA.
  struct TrailStyle {
    Vec4f head { 1.0f, 1.0f, 1.0f, 1.0f };  //!< at the body
    Vec4f tail { 0.25f, 0.5f, 1.0f, 0.0f }; //!< at the far end; alpha 0 fades the trail out
    float headWidth { 2.0f };               //!< ribbon width in pixels at the body
    float tailWidth { 0.5f };               //!< ribbon width in pixels at the far end
  };

  /*!
   * Builds every trail in the cloud as one indexed triangle batch.
   *
   * The obvious shape - one draw per trail - costs a draw call per body, and nothing can merge
   * them because a line strip would join trails that are nowhere near each other. Expanding each
   * trail into a triangle ribbon instead makes the whole cloud one indexed batch whose cost is
   * independent of how many bodies there are, and buys per-vertex color, so the trail can fade
   * along its length for free.
   *
   * This used to hand the batch straight to `SDL_RenderGeometry`. It now only builds it, and the
   * caller passes it up as a `DrawCommand` - which is why the demo no longer links SDL at all.
   *
   * The vertex and index buffers are members rather than locals so they keep their capacity
   * between frames, and so the pointers a `DrawCommand` carries stay valid for the whole render
   * pass; the indices only depend on the buffer's shape, so they are rebuilt only while the ring
   * is still filling and left alone after that.
   */
  class TrailRenderer {
  public:
    void Build(const TrailBuffer &trails, const TrailStyle &style);

    [[nodiscard]] const List<DrawVertex> &Vertices() const noexcept { return m_vertices; }
    [[nodiscard]] const List<uint32_t> &Indices() const noexcept { return m_indices; }

    [[nodiscard]] size_t VertexCount() const noexcept { return m_vertices.size(); }
    [[nodiscard]] size_t TriangleCount() const noexcept { return m_indices.size() / 3; }

  private:
    void buildIndices(uint32_t trails, uint32_t samples);

    List<DrawVertex> m_vertices;
    List<uint32_t> m_indices;
    uint32_t m_indexedTrails { 0 }; //!< shape the current index buffer was built for
    uint32_t m_indexedSamples { 0 };
  };
} // namespace Orbits
