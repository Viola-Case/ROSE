/**

    @file      pointcloud.h
    @brief     N-body-ish point cloud: integration, and the glue to the trail renderer.
    @details   ~
    @author    Viola Case
    @date      26.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <ROSE/ROSE.h>

#include "trailrenderer.h"

namespace Orbits {
  using namespace ROSE;

  /*!
   * A `Renderable`, so the cloud hands its geometry up rather than drawing it.
   *
   * Nothing in here knows which backend is running - there is no SDL renderer to resolve, no
   * SDL call, and no SDL header - which is the whole point: the same demo runs on SDL's
   * renderer, the software rasterizer and OpenGL with no source change.
   */
  class PointCloud : public Renderable {
  public:
    static constexpr UUID typeID = "6592694121c0a7d9-ea7b9a70926599a7"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return typeID; }

    PointCloud() noexcept;

  protected:
    void FrameUpdate() override;
    void Unpack(const ParamView &) override;
    void Collect(RenderList &) override;

  private:
    void integrate(double dt);

    //! Advances the trails, projects to window space, and rebuilds the geometry Collect emits.
    void updateGeometry();

    List<Vec3d> m_positions;
    List<Vec3d> m_velocities;

    TrailBuffer m_trails;
    TrailRenderer m_trailRenderer;
    TrailStyle m_trailStyle;

    List<Point> m_screen; //!< positions in window space; sized once in Unpack, overwritten each frame

    /* What Collect hands to the backend. Members, not locals: a DrawCommand keeps the pointer
     * for the whole render pass. */
    List<DrawVertex> m_bodyVertices;
    DrawVertex m_centerVertex {};
  };
} // namespace Orbits
