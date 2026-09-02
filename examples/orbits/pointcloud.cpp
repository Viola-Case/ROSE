/**

    @file      pointcloud.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      26.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include "pointcloud.h"

#include <ROSE/Core/imgui.h>

#include <random>

namespace Orbits {
  namespace {
    /* Fractions of the circular-orbit speed at whatever radius a body starts at, rather than
     * absolute speeds: the speed that holds an orbit together is sqrt(-G / r), so a fixed
     * pixels-per-second range means something different at r = 40 than at r = 200. Below 1 gives
     * an ellipse falling inward, above 1 one swinging out; far off either way and the body just
     * leaves. */
    constexpr double MIN_ORBIT_FRACTION = 0.8;
    constexpr double MAX_ORBIT_FRACTION = 1.1;
    constexpr double GRAV_CONSTANT = -3000000.0;
    constexpr double MIN_POS_RAD = 40.0;
    constexpr double MAX_POS_RAD = 200.0;

    constexpr int DEFAULT_COUNT = 100;
    constexpr int DEFAULT_TRAIL = 64;
    constexpr int MIN_TRAIL = 2; //!< a ribbon needs two samples before it has a segment

    constexpr unsigned int ITERATIONS = 3;

    constexpr Vec4f BODY_COLOR { 1.0f, 1.0f, 1.0f, 1.0f };
    constexpr Vec4f CENTER_COLOR { 0.0f, 1.0f, 1.0f, 1.0f };
  } // namespace

  /* Everything this draws is already in window pixels, so it skips the camera. Transparent puts
   * all three commands in one band, where they keep the order Collect adds them in - ribbon,
   * then bodies, then the centre on top. Opaque would have sorted the bodies *under* the
   * ribbon, which is the one thing the banding would otherwise get wrong here. */
  PointCloud::PointCloud() noexcept { m_flags = RENDERABLE_SCREEN_SPACE | RENDERABLE_TRANSPARENT; }

  void PointCloud::Unpack(const ParamView &view) {
    const int seed = view.GetInt("seed", 0);
    const int count = view.GetInt("count", DEFAULT_COUNT);
    const int trail = view.GetInt("trail", DEFAULT_TRAIL);

    if (count <= 0 || trail < MIN_TRAIL) {
      ROSE_LOG_ERROR("Orbits: bad point cloud parameters (count {}, trail {}); nothing to simulate.\n", count, trail);
      return;
    }

    std::mt19937 generate(seed);
    std::uniform_real_distribution<> theta(0.0, math::TAU);
    std::uniform_real_distribution<> posMag(MIN_POS_RAD, MAX_POS_RAD);
    std::uniform_real_distribution<> orbitFraction(MIN_ORBIT_FRACTION, MAX_ORBIT_FRACTION);

    m_positions.reserve(count);
    m_velocities.reserve(count);
    m_screen.resize(count);
    m_bodyVertices.resize(count);
    m_trails.Reset(static_cast<uint32_t>(count), static_cast<uint32_t>(trail));

    for (int i = 0; i < count; i++) {
      const double t = theta(generate);
      const double m = posMag(generate);
      const Vec3d pos { math::Cos(t) * m, math::Sin(t) * m, 0 };

      m_positions.emplace_back(pos);
      /* A tangential launch needs the *unit* normal scaled by the speed. `Norm()` returns the
       * magnitude, so the old `.Norm() * m` was a plain double, and Vec3d's per-component
       * constructor read it as (|p x z| * m, 0, 0) - every body set off along +x instead. */
      const double speed = math::Sqrt(-GRAV_CONSTANT / m) * orbitFraction(generate);
      m_velocities.emplace_back(Vec3d::CrossProduct(pos, Vec3d { 0, 0, 1 }).Unit() * speed);
    }
  }

  void PointCloud::FrameUpdate() {
    /* Hoisted out of the substep loop: the two lists are only ever sized together in Unpack, so
     * re-checking them once per iteration proved nothing the first check had not. */
    if (m_positions.size() != m_velocities.size()) {
      ROSE_LOG_ERROR("POINT CLOUD SIZE MISMATCH!\npoints size: {}\nvelocities size: {}\n", m_positions.size(),
                     m_velocities.size());
      GetScene().GetApplication().Quit();
      return;
    }

    const double dt = Time::deltaTime / static_cast<double>(ITERATIONS);
    for (auto it = 0u; it < ITERATIONS; it++) {
      integrate(dt);
    }

    updateGeometry();
  }

  void PointCloud::integrate(double dt) {
    for (size_t i = 0; i < m_positions.size(); ++i) {
      Vec3d &pos = m_positions[i];
      Vec3d &vel = m_velocities[i];
      pos += vel * dt;
      vel += pos.Unit() * (GRAV_CONSTANT * dt / Vec3d::DotProduct(pos, pos));
    }
  }

  void PointCloud::updateGeometry() {
    // Cached on the Window, so this costs nothing and beats assuming the launch size forever.
    const Window *window = GetScene().GetApplication().GetWindow();
    if (!window) return;

    const math::Vec2<int> size = window->GetSize();
    const float centerX = static_cast<float>(size.x) * 0.5f;
    const float centerY = static_cast<float>(size.y) * 0.5f;

    m_trails.Advance();
    for (size_t i = 0; i < m_positions.size(); ++i) {
      const Point p { static_cast<float>(m_positions[i].x) + centerX, static_cast<float>(m_positions[i].y) + centerY };
      m_screen[i] = p;
      m_trails.Write(static_cast<uint32_t>(i), p);
      m_bodyVertices[i] = DrawVertex { { p.x, p.y, 0.0f }, BODY_COLOR, {} };
    }

    m_trailRenderer.Build(m_trails, m_trailStyle);
    m_centerVertex = DrawVertex { { centerX, centerY, 0.0f }, CENTER_COLOR, {} };

    /* Replaces the per-frame Log of every trail's length, which was a formatted write per body
     * per frame and cost more than everything above it put together. */
    const ImGuiIO &io = ImGui::GetIO();
    if (ImGui::Begin("Orbits")) {
      ImGui::Text("%.1f fps (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
      ImGui::Text("bodies  %zu", m_positions.size());
      ImGui::Text("trail   %u / %u samples", m_trails.Filled(), m_trails.Length());
      ImGui::Text("ribbon  %zu verts, %zu tris", m_trailRenderer.VertexCount(), m_trailRenderer.TriangleCount());
    }
    ImGui::End();

    /*
    ======== THE TEMPORARY CODE BLOCK OF SHAME ========
    Claude Code added this to benchmark the trails then forgot to remove it afterward, ending up in the repo.
    Let this be a lesson unto you: LLMs are dumb.

    {
      static int frames = 0;
      static double acc = 0.0;
      ++frames;
      if (frames > 100) acc += Time::deltaTime;
      if (frames == 700) {
        if (FILE *f = std::fopen("C:/Users/my_actual_name_wtf_claude/AppData/Local/Temp/bench_C.txt", "w")) {
          std::fprintf(f, "%.4f\n", acc / 600.0 * 1000.0);
          std::fclose(f);
        }
        m_object->GetScene().GetApplication().Quit();
      }
    }
    ===================================================
    */
  }

  void PointCloud::Collect(RenderList &out) {
    // Order matters and is preserved: same band, same layer, so these draw as added.
    if (!m_trailRenderer.Vertices().empty()) {
      DrawCommand ribbon;
      ribbon.vertices = m_trailRenderer.Vertices().data();
      ribbon.vertexCount = m_trailRenderer.Vertices().size();
      ribbon.indices = m_trailRenderer.Indices().data();
      ribbon.indexCount = m_trailRenderer.Indices().size();
      ribbon.topology = Topology::Triangles;
      ribbon.flags = m_flags;
      out.Add(ribbon);
    }

    if (!m_bodyVertices.empty()) {
      DrawCommand bodies;
      bodies.vertices = m_bodyVertices.data();
      bodies.vertexCount = m_bodyVertices.size();
      bodies.topology = Topology::Points;
      bodies.flags = m_flags;
      out.Add(bodies);
    }

    DrawCommand center;
    center.vertices = &m_centerVertex;
    center.vertexCount = 1;
    center.topology = Topology::Points;
    center.flags = m_flags;
    out.Add(center);
  }
} // namespace Orbits
