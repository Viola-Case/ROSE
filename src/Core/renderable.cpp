/**

  @file       renderable.cpp
  @brief      Enrollment, and every renderable's geometry.
  @details    ~
  @author     Viola Case
  @date       29.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>

namespace ROSE {

  namespace {
    /*! Two triangles over a quad wound 0-1-2, 0-2-3. Static storage: a `DrawCommand` keeps the
     *  pointer for the whole pass, so this must not be a local. */
    constexpr uint32_t kQuadIndices[6] { 0, 1, 2, 0, 2, 3 };

    constexpr Vec3f ToVec3f(const Vec3d &_v) noexcept {
      return { static_cast<float>(_v.x), static_cast<float>(_v.y), static_cast<float>(_v.z) };
    }
  } // namespace

#pragma region Renderable

  void Renderable::OnCreate() {
    /* By the time OnCreate runs, Scene::Bind has already pointed the scene at its application:
     * OnCreate is only ever reached from Scene::OnStart or Scene::InitializePendingBehaviors,
     * both of which run inside Application::Run or Application::Init, after Bind. A behavior
     * built but never attached has no object, and simply never enrolls. */
    if (!m_object) return;

    if (RenderBackend *backend = GetScene().GetApplication().GetRenderBackend())
      backend->Enroll(*this);
  }

  void Renderable::OnDestroy() {
    if (m_renderer) m_renderer->Withdraw(*this);
  }

  Renderable::~Renderable() {
    /* Belt and braces: OnDestroy is the ordinary path, but a Renderable that is simply deleted -
     * or one whose scene outlived its backend - must still leave the registry consistent.
     * RenderBackend::DetachAllRenderables covers the mirror case, where the backend dies first. */
    if (m_renderer) m_renderer->Withdraw(*this);
  }

  Mat4f Renderable::ModelMatrix() const noexcept {
    if (!m_object) return Mat4f::Identity();

    const Transform &t = m_object->transform;
    const Quatf rotation { static_cast<float>(t.rotation.w), static_cast<float>(t.rotation.x),
                           static_cast<float>(t.rotation.y), static_cast<float>(t.rotation.z) };

    // Column-vector convention, so the rightmost factor applies first: scale, rotate, translate.
    return Mat4f::Translation(ToVec3f(t.position)) * rotation.ToMat4() * Mat4f::Scaling(ToVec3f(t.scale));
  }

#pragma endregion

#pragma region MeshRenderable

  void MeshRenderable::SetMesh(const UUID &_id) noexcept {
    m_meshID = _id;
    m_mesh = MeshRegistry::Get().GetMesh(_id);
    m_dirty = true;
  }

  void MeshRenderable::SetTint(const Vec4f &_tint) noexcept {
    m_tint = _tint;
    m_dirty = true;
  }

  void MeshRenderable::Rebuild() noexcept {
    m_dirty = false;
    m_vertices.clear();
    if (!m_mesh) return;

    m_vertices.reserve(m_mesh->vertices.size());
    for (const Vert &v : m_mesh->vertices) {
      DrawVertex dv;
      dv.position = v.position;
      dv.color = m_tint;
      dv.texCoord = v.texCoord;
      m_vertices.push_back(dv);
    }
  }

  void MeshRenderable::Unpack(const ParamView &_view) {
    m_meshID = _view.GetUUID("mesh");
    m_meshName = _view.GetString("meshName", "");
    const Vec4d tint = _view.GetVec4d("tint", { 1.0, 1.0, 1.0, 1.0 });
    m_tint = { static_cast<float>(tint.x), static_cast<float>(tint.y), static_cast<float>(tint.z),
               static_cast<float>(tint.w) };
    m_layer = _view.GetInt("layer", 0);
    if (_view.GetBool("transparent", false)) m_flags |= RENDERABLE_TRANSPARENT;
    m_dirty = true;
    /* Neither the id nor the name is resolved here: Unpack runs at JSON parse time, before
     * anything has had a chance to register meshes. `Collect` retries until it lands. */
  }

  void MeshRenderable::Collect(RenderList &_out) {
    if (!m_mesh) { // late registration; the mesh may not have existed when the scene was parsed
      if (m_meshID == UUID::Invalid() && !m_meshName.empty())
        m_meshID = MeshRegistry::Get().GetMeshID(m_meshName);
      if (m_meshID != UUID::Invalid()) SetMesh(m_meshID);
    }
    if (!m_mesh) return;
    if (m_dirty) Rebuild();
    if (m_vertices.empty() || m_mesh->indices.empty()) return;

    DrawCommand cmd;
    cmd.vertices = m_vertices.data();
    cmd.vertexCount = m_vertices.size();
    cmd.indices = m_mesh->indices.data();
    cmd.indexCount = m_mesh->indices.size();
    cmd.topology = Topology::Triangles;
    cmd.flags = m_flags;
    cmd.transform = ModelMatrix();
    _out.Add(cmd);
  }

#pragma endregion

#pragma region SpriteRenderable

  SpriteRenderable::SpriteRenderable() noexcept { m_flags = RENDERABLE_TEXTURED | RENDERABLE_TRANSPARENT; }

  void SpriteRenderable::SetSize(const Vec2f &_size) noexcept {
    m_size = _size;
    m_dirty = true;
  }

  void SpriteRenderable::SetTint(const Vec4f &_tint) noexcept {
    m_tint = _tint;
    m_dirty = true;
  }

  void SpriteRenderable::Rebuild() noexcept {
    m_dirty = false;

    // Centred on the object's origin, in the local XY plane, v pointing down the way images do.
    const float hw = m_size.x * 0.5f;
    const float hh = m_size.y * 0.5f;

    m_vertices[0] = { { -hw, hh, 0.f }, m_tint, { 0.f, 0.f } };
    m_vertices[1] = { { hw, hh, 0.f }, m_tint, { 1.f, 0.f } };
    m_vertices[2] = { { hw, -hh, 0.f }, m_tint, { 1.f, 1.f } };
    m_vertices[3] = { { -hw, -hh, 0.f }, m_tint, { 0.f, 1.f } };
  }

  void SpriteRenderable::Unpack(const ParamView &_view) {
    m_texture = _view.GetUUID("texture");
    const Vec3d size = _view.GetVec3d("size", { 1.0, 1.0, 0.0 });
    m_size = { static_cast<float>(size.x), static_cast<float>(size.y) };
    const Vec4d tint = _view.GetVec4d("tint", { 1.0, 1.0, 1.0, 1.0 });
    m_tint = { static_cast<float>(tint.x), static_cast<float>(tint.y), static_cast<float>(tint.z),
               static_cast<float>(tint.w) };
    m_layer = _view.GetInt("layer", 0);
    m_dirty = true;
  }

  void SpriteRenderable::Collect(RenderList &_out) {
    if (m_dirty) Rebuild();

    DrawCommand cmd;
    cmd.vertices = m_vertices;
    cmd.vertexCount = 4;
    cmd.indices = kQuadIndices;
    cmd.indexCount = 6;
    cmd.topology = Topology::Triangles;
    cmd.texture = m_texture;
    cmd.flags = m_flags;
    cmd.transform = ModelMatrix();
    _out.Add(cmd);
  }

#pragma endregion

#pragma region ShapeRenderable

  void ShapeRenderable::SetPoints(const List<Vec3f> &_points) noexcept {
    m_points = _points;
    m_dirty = true;
  }

  void ShapeRenderable::SetColor(const Vec4f &_color) noexcept {
    m_color = _color;
    m_dirty = true;
  }

  void ShapeRenderable::SetWidth(const float _width) noexcept {
    m_width = _width;
    m_dirty = true;
  }

  void ShapeRenderable::SetClosed(const bool _closed) noexcept {
    m_closed = _closed;
    m_dirty = true;
  }

  void ShapeRenderable::Rebuild() noexcept {
    m_dirty = false;
    m_vertices.clear();
    m_indices.clear();
    if (m_points.size() < 2) return;

    const size_t count = m_points.size();
    const size_t segments = m_closed ? count : count - 1;

    if (m_width <= 1.f) {
      // Thin: disjoint segments, two vertices each. See Topology::Lines.
      m_vertices.reserve(count);
      for (const Vec3f &p : m_points)
        m_vertices.push_back(DrawVertex { p, m_color, { 0.f, 0.f } });

      m_indices.reserve(segments * 2);
      for (size_t i = 0; i < segments; ++i) {
        m_indices.push_back(static_cast<uint32_t>(i));
        m_indices.push_back(static_cast<uint32_t>((i + 1) % count));
      }
      return;
    }

    /* Wide: expand to a triangle ribbon here rather than ask the backend for a line width it
     * cannot portably honour. The offset is perpendicular in XY, which is what a 2D shape wants
     * and what a 3D one would need a camera-facing basis to improve on. */
    const float half = m_width * 0.5f;
    m_vertices.reserve(segments * 4);
    m_indices.reserve(segments * 6);

    for (size_t i = 0; i < segments; ++i) {
      const Vec3f &a = m_points[i];
      const Vec3f &b = m_points[(i + 1) % count];

      const float dx = b.x - a.x;
      const float dy = b.y - a.y;
      const float length = math::Sqrt(dx * dx + dy * dy);
      if (!(length > 0.f)) continue; // coincident points contribute no quad

      const float nx = -dy / length * half;
      const float ny = dx / length * half;

      const auto base = static_cast<uint32_t>(m_vertices.size());
      m_vertices.push_back(DrawVertex { { a.x + nx, a.y + ny, a.z }, m_color, { 0.f, 0.f } });
      m_vertices.push_back(DrawVertex { { b.x + nx, b.y + ny, b.z }, m_color, { 1.f, 0.f } });
      m_vertices.push_back(DrawVertex { { b.x - nx, b.y - ny, b.z }, m_color, { 1.f, 1.f } });
      m_vertices.push_back(DrawVertex { { a.x - nx, a.y - ny, a.z }, m_color, { 0.f, 1.f } });

      for (const uint32_t offset : kQuadIndices)
        m_indices.push_back(base + offset);
    }
  }

  void ShapeRenderable::Unpack(const ParamView &_view) {
    const Vec4d color = _view.GetVec4d("color", { 1.0, 1.0, 1.0, 1.0 });
    m_color = { static_cast<float>(color.x), static_cast<float>(color.y), static_cast<float>(color.z),
                static_cast<float>(color.w) };
    m_width = static_cast<float>(_view.GetDouble("width", 1.0));
    m_closed = _view.GetBool("closed", false);
    m_layer = _view.GetInt("layer", 0);
    if (_view.GetBool("transparent", false)) m_flags |= RENDERABLE_TRANSPARENT;
    if (_view.GetBool("screenSpace", false)) m_flags |= RENDERABLE_SCREEN_SPACE;
    m_dirty = true;
    /* Points are not read from JSON yet - a shape is driven from code today. A point list in the
     * scene file wants an array-of-arrays reader on ParamView first. */
  }

  void ShapeRenderable::Collect(RenderList &_out) {
    if (m_dirty) Rebuild();
    if (m_vertices.empty() || m_indices.empty()) return;

    DrawCommand cmd;
    cmd.vertices = m_vertices.data();
    cmd.vertexCount = m_vertices.size();
    cmd.indices = m_indices.data();
    cmd.indexCount = m_indices.size();
    cmd.topology = m_width <= 1.f ? Topology::Lines : Topology::Triangles;
    cmd.flags = m_flags;
    cmd.transform = ModelMatrix();
    _out.Add(cmd);
  }

#pragma endregion

#pragma region GeometryRenderable

  void GeometryRenderable::Unpack(const ParamView &_view) {
    m_layer = _view.GetInt("layer", 0);
    if (_view.GetBool("transparent", false)) m_flags |= RENDERABLE_TRANSPARENT;
    if (_view.GetBool("screenSpace", false)) m_flags |= RENDERABLE_SCREEN_SPACE;
    if (_view.GetBool("overlay", false)) m_flags |= RENDERABLE_OVERLAY;
    pointSize = static_cast<float>(_view.GetDouble("pointSize", 1.0));
    texture = _view.GetUUID("texture");
    if (texture != UUID::Invalid()) m_flags |= RENDERABLE_TEXTURED;
  }

  void GeometryRenderable::Collect(RenderList &_out) {
    if (vertices.empty()) return;

    DrawCommand cmd;
    cmd.vertices = vertices.data();
    cmd.vertexCount = vertices.size();
    cmd.indices = indices.empty() ? nullptr : indices.data();
    cmd.indexCount = indices.size();
    cmd.topology = topology;
    cmd.texture = texture;
    cmd.flags = m_flags;
    cmd.transform = ModelMatrix();
    cmd.pointSize = pointSize;
    _out.Add(cmd);
  }

#pragma endregion

#pragma region UI

  UIRenderable::UIRenderable() noexcept { m_flags = RENDERABLE_OVERLAY | RENDERABLE_SCREEN_SPACE; }

  void UIRenderable::SetRect(const Vec2f &_position, const Vec2f &_size) noexcept {
    m_position = _position;
    m_size = _size;
  }

  void UIRenderable::Unpack(const ParamView &_view) {
    const Vec3d position = _view.GetVec3d("position", { 0.0, 0.0, 0.0 });
    const Vec3d size = _view.GetVec3d("size", { 0.0, 0.0, 0.0 });
    m_position = { static_cast<float>(position.x), static_cast<float>(position.y) };
    m_size = { static_cast<float>(size.x), static_cast<float>(size.y) };
    m_layer = _view.GetInt("layer", 0);
  }

  void TextUI::Unpack(const ParamView &_view) {
    UIRenderable::Unpack(_view);
    m_text = _view.GetString("text", "");
    const Vec4d color = _view.GetVec4d("color", { 1.0, 1.0, 1.0, 1.0 });
    m_color = { static_cast<float>(color.x), static_cast<float>(color.y), static_cast<float>(color.z),
                static_cast<float>(color.w) };
  }

  ImageUI::ImageUI() noexcept { m_flags |= RENDERABLE_TEXTURED | RENDERABLE_TRANSPARENT; }

  void ImageUI::Unpack(const ParamView &_view) {
    UIRenderable::Unpack(_view);
    m_texture = _view.GetUUID("texture");
    const Vec4d tint = _view.GetVec4d("tint", { 1.0, 1.0, 1.0, 1.0 });
    m_tint = { static_cast<float>(tint.x), static_cast<float>(tint.y), static_cast<float>(tint.z),
               static_cast<float>(tint.w) };
  }

  void ImageUI::Collect(RenderList &_out) {
    if (!(m_size.x > 0.f) || !(m_size.y > 0.f)) return;

    // Screen space: window pixels, top-left origin, so the rect is used as written.
    const float l = m_position.x;
    const float t = m_position.y;
    const float r = l + m_size.x;
    const float b = t + m_size.y;

    m_vertices[0] = { { l, t, 0.f }, m_tint, { 0.f, 0.f } };
    m_vertices[1] = { { r, t, 0.f }, m_tint, { 1.f, 0.f } };
    m_vertices[2] = { { r, b, 0.f }, m_tint, { 1.f, 1.f } };
    m_vertices[3] = { { l, b, 0.f }, m_tint, { 0.f, 1.f } };

    DrawCommand cmd;
    cmd.vertices = m_vertices;
    cmd.vertexCount = 4;
    cmd.indices = kQuadIndices;
    cmd.indexCount = 6;
    cmd.topology = Topology::Triangles;
    cmd.texture = m_texture;
    cmd.flags = m_flags;
    _out.Add(cmd);
  }

#pragma endregion

} // namespace ROSE
