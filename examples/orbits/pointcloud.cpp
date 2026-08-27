/**

    @file      pointcloud.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      26.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include "pointcloud.h"
#include <random>
#include <SDL3/SDL.h>

constexpr double MIN_STARTING_SPEED = 5.0;
constexpr double MAX_STARTING_SPEED = 100.0;
constexpr double GRAV_CONSTANT = -3000000.0;
constexpr double MIN_POS_RAD = 40;
constexpr double MAX_POS_RAD = 200;

constexpr unsigned int ITERATIONS = 100;

namespace Orbits {
  void PointCloud::Unpack(const ParamView &view) {
    const auto seed = view.GetInt("seed", 0);
    std::mt19937 generate(seed);
    std::uniform_real_distribution<> theta(0.0, math::TAU);
    std::uniform_real_distribution<> posMag(MIN_POS_RAD, MAX_POS_RAD);
    std::uniform_real_distribution<> velMag(MIN_STARTING_SPEED, MAX_STARTING_SPEED);
    auto count = view.GetInt("count", 100);
    m_points.reserve(count);
    m_velocities.reserve(count);
    for (auto i = 0; i < count; i++) {
      double t = theta(generate);
      double m = posMag(generate);
      m_points.emplace_back(Vec3d { math::Cos(t) * m, math::Sin(t) * m, 0 });
      // t = theta(generate);
      // m = velMag(generate);
      // m_velocities.emplace_back(Vec3d { math::Cos(t) * m, math::Sin(t) * m, 0 });
      m_velocities.emplace_back(
        Vec3d::CrossProduct(m_points[i], Vec3d { 0, 0, 1 })
        );
    }
  }
  void PointCloud::OnCreate() {}
  void PointCloud::FrameUpdate() {
    constexpr double ITERATION_CONSTANT = 1. / static_cast<double>(ITERATIONS);
    for (auto it = 0u; it < ITERATIONS; it++) {
      // Log(LogLevel::Info, "Iteration {}\n", it);
      if (m_points.size() != m_velocities.size()) {
        ROSE_LOG_ERROR("POINT CLOUD SIZE MISMATCH!\npoints size: {}\nvelocities size: {}", m_points.size(),
                       m_velocities.size());
        m_object->GetScene().GetApplication().Quit();
        return;
      }
      for (auto idx = 0; idx < m_points.size(); idx++) {
        Vec3d &pos = m_points[idx];
        Vec3d &vel = m_velocities[idx];
        pos += vel * Time::deltaTime * ITERATION_CONSTANT;
        vel += pos.Unit() * (GRAV_CONSTANT * 1. / Vec3d::DotProduct(pos, pos) * Time::deltaTime * ITERATION_CONSTANT);
      }
    }
    Log(LogLevel::Info, "Point 1 \n\tposition:{:|.3f} \n\tvelocity:{:|.3f}\n", m_points[0], m_velocities[0]);
    renderCloud();
  }

  void PointCloud::renderCloud() {
    static List<Point> points {};
    points.clear();
    for (auto i = 0; i < m_points.size(); i++) {
      Point p { static_cast<float>(m_points[i].x) + 400.f, static_cast<float>(m_points[i].y) + 400.f };

      points.emplace_back(p);
    }
    static SDL_Renderer *renderer =
      SDL_GetRenderer(static_cast<SDL_Window *>(m_object->GetScene().GetApplication().GetWindow()->GetHandle()));

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderPoints(renderer, reinterpret_cast<SDL_FPoint *>(points.data()), points.size());

    SDL_SetRenderDrawColor(renderer, 0, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderPoint(renderer, 400.f, 400.f);
  }
} // namespace Orbits
