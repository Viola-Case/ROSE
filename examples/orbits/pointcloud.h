/**

    @file      pointcloud.h
    @brief
    @details   ~
    @author    Viola Case
    @date      26.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <ROSE/ROSE.h>

using namespace ROSE;

namespace Orbits {
  struct Point {
    float x, y;
  };

  class PointCloud : public Behavior {
  public:
    static constexpr UUID typeID = "6592694121c0a7d9-ea7b9a70926599a7"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return typeID; }

  protected:
    void OnCreate() override;
    //void OnStart() override;
    void FrameUpdate() override;
    void Unpack(const ParamView &) override;

  private:
    List<Vec3d> m_points;
    List<Vec3d> m_velocities;
    void renderCloud();
  };


} // namespace