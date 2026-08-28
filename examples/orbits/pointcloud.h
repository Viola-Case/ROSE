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

  class PointCloud : public Behavior {
  public:
    static constexpr UUID typeID = "6592694121c0a7d9-ea7b9a70926599a7"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return typeID; }

  protected:
    void OnStart() override;
    void FrameUpdate() override;
    void Unpack(const ParamView &) override;

  private:
    void integrate(double dt);
    void render();

    List<Vec3d> m_positions;
    List<Vec3d> m_velocities;

    TrailBuffer m_trails;
    TrailRenderer m_trailRenderer;
    TrailStyle m_trailStyle;

    List<Point> m_screen; //!< positions in window space; sized once in Unpack, overwritten each frame

    SDL_Renderer *m_renderer { nullptr }; //!< resolved once in OnStart
  };
} // namespace Orbits
