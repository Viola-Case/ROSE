/**

  @file       renderable.h
  @brief
  @details    ~
  @author     Viola Case
  @date       10.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/behavior.h>
#include <ROSE/Core/gfx.h>
#include <ROSE/Core/mesh.h>

namespace ROSE {

  /*!
   * A behavior that contributes geometry to the frame.
   *
   * The contract runs one way: a renderable produces plain data and never learns which backend
   * consumes it. There is no native handle, no backend type, and no per-backend code path
   * anywhere below this class, which is what lets a new backend be added without touching a
   * single behavior.
   *
   * Enrollment is automatic. `OnCreate` registers with the application's backend and `OnDestroy`
   * withdraws, so a renderable is drawn for exactly as long as it exists. Disabling one keeps it
   * enrolled - the render pass simply skips it - so toggling visibility costs nothing.
   */
  class ROSE_API(CORE) Renderable : public Behavior {
    friend class RenderBackend;

  public:
    [[nodiscard]] RenderableFlags GetRenderableFlags() const noexcept { return m_flags; }

    /*! Draw order within a band. Lower draws first; ties fall back to enrollment order. */
    [[nodiscard]] int32_t GetLayer() const noexcept { return m_layer; }
    void SetLayer(const int32_t _layer) noexcept { m_layer = _layer; }

    [[nodiscard]] bool IsEnrolled() const noexcept { return m_renderer != nullptr; }

  protected:
    /*!
     * Enrolls with the application's backend.
     *
     * A subclass that overrides this **must** call `Renderable::OnCreate()`. Safe with no
     * backend at all (a headless run): enrollment is skipped and the renderable is simply never
     * drawn, rather than being an error.
     */
    void OnCreate() override;
    void OnDestroy() override; //!< withdraws
    ~Renderable() override;    //!< withdraws if still enrolled, so a bare delete is safe

    /*!
     * Fill @p _out with this frame's geometry.
     *
     * Called once per frame by the render pass, after every behavior's `FrameUpdate` and only
     * while `IsEnabled()`. Add nothing to sit the frame out.
     *
     * Whatever a command points at must stay put until the frame ends - the backend may defer
     * the actual drawing - so emit from storage owned by the renderable, not from a local.
     */
    virtual void Collect(RenderList &_out) = 0;

    /*!
     * The owning object's transform as an object-to-world matrix, T * R * S.
     *
     * Identity when the renderable is not attached to anything. `Object::m_parent` is never
     * assigned, so there is no parent composition to do yet.
     */
    [[nodiscard]] Mat4f ModelMatrix() const noexcept;

    RenderableFlags m_flags {};
    int32_t m_layer { 0 };

  private:
    RenderBackend *m_renderer { nullptr }; //!< the backend this is enrolled with, if any
    size_t m_slot { 0 };                   //!< index into `RenderBackend::m_renderables`
  };

  /*!
   * A mesh from the `MeshRegistry`, drawn as indexed triangles.
   */
  class ROSE_API(CORE) MeshRenderable : public Renderable {
  public:
    static constexpr UUID typeID = "9cfaac61886d5e2e-9e75d84332659afc"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    void SetMesh(const UUID &_id) noexcept;
    void SetTint(const Vec4f &_tint) noexcept;

  protected:
    void Unpack(const ParamView &) override;
    void Collect(RenderList &) override;

  private:
    /*! `Mesh` stores `Vec3d`; the backends want `Vec3f`. Converted once per mesh or tint change. */
    void Rebuild() noexcept;

    UUID m_meshID {};
    const Mesh *m_mesh { nullptr };
    Vec4f m_tint { 1.f, 1.f, 1.f, 1.f };
    List<DrawVertex> m_vertices {};
    bool m_dirty { true };
  };

  /*!
   * A textured quad in world space.
   */
  class ROSE_API(CORE) SpriteRenderable : public Renderable {
  public:
    static constexpr UUID typeID = "32c5294ce2e000fd-271098999265987f"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    SpriteRenderable() noexcept;

    void SetTexture(const TextureID &_id) noexcept { m_texture = _id; }
    void SetSize(const Vec2f &_size) noexcept;
    void SetTint(const Vec4f &_tint) noexcept;

  protected:
    void Unpack(const ParamView &) override;
    void Collect(RenderList &) override;

  private:
    void Rebuild() noexcept;

    TextureID m_texture {};
    Vec2f m_size { 1.f, 1.f };
    Vec4f m_tint { 1.f, 1.f, 1.f, 1.f };
    DrawVertex m_vertices[4] {};
    bool m_dirty { true };
  };

  /*!
   * A polyline through a list of points. Width above one pixel is expanded into a triangle
   * ribbon here rather than asked of the backend, because line width is not portable.
   */
  class ROSE_API(CORE) ShapeRenderable : public Renderable {
  public:
    static constexpr UUID typeID = "4e503b30bef7e6ef-a5250bb2d5659727"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    void SetPoints(const List<Vec3f> &_points) noexcept;
    void SetColor(const Vec4f &_color) noexcept;
    void SetWidth(float _width) noexcept;
    void SetClosed(bool _closed) noexcept;

  protected:
    void Unpack(const ParamView &) override;
    void Collect(RenderList &) override;

  private:
    void Rebuild() noexcept;

    List<Vec3f> m_points {};
    Vec4f m_color { 1.f, 1.f, 1.f, 1.f };
    float m_width { 1.f };
    bool m_closed { false };

    List<DrawVertex> m_vertices {};
    List<uint32_t> m_indices {};
    bool m_dirty { true };
  };

  /*!
   * Arbitrary geometry, supplied by whoever owns the behavior.
   *
   * The general batched primitive and the escape hatch that is not an escape hatch: anything the
   * typed renderables above do not cover is expressed as vertices and indices here, which every
   * backend already understands. Fill `vertices` / `indices` from `FrameUpdate` and they are
   * drawn as-is.
   */
  class ROSE_API(CORE) GeometryRenderable : public Renderable {
  public:
    static constexpr UUID typeID = "1f6b0c7d94a3e582-b0d47e2c3165a91d"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    /* Public and owned by the caller. Stable for the frame as long as they are not touched
     * between the render pass and the next FrameUpdate. */
    List<DrawVertex> vertices {};
    List<uint32_t> indices {}; //!< empty draws `vertices` unindexed
    Topology topology { Topology::Triangles };
    TextureID texture {};
    float pointSize { 1.f };

    void SetRenderableFlags(const RenderableFlags _flags) noexcept { m_flags = _flags; }

  protected:
    void Unpack(const ParamView &) override;
    void Collect(RenderList &) override;
  };

  /*!
   * Base for screen-space UI. Concrete and registered in its own right - it has a type ID a
   * scene file can name - but it draws nothing on its own; the leaves below supply the geometry.
   * It owns the rect and the screen-space flags they share.
   */
  class ROSE_API(CORE) UIRenderable : public Renderable {
  public:
    static constexpr UUID typeID = "cb497ab81ab3dd7b-85272b1b21659b69"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    UIRenderable() noexcept;

    void SetRect(const Vec2f &_position, const Vec2f &_size) noexcept;

  protected:
    void Unpack(const ParamView &) override;
    void Collect(RenderList &) override {}

    Vec2f m_position { 0.f, 0.f }; //!< window pixels, top-left origin
    Vec2f m_size { 0.f, 0.f };
  };

  /*!
   * @todo Draws nothing yet. Real text needs a font atlas built on SDL_ttf, which is its own
   *       piece of work; the type, its ID and its parameters exist so scenes that already name
   *       it keep loading.
   */
  class ROSE_API(CORE) TextUI : public UIRenderable {
  public:
    static constexpr UUID typeID = "9f24e65aa0a93483-992866975b659b59"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

  protected:
    void Unpack(const ParamView &) override;
    void Collect(RenderList &) override {}

  private:
    String m_text {};
    Vec4f m_color { 1.f, 1.f, 1.f, 1.f };
  };

  /*!
   * A textured quad in screen space.
   */
  class ROSE_API(CORE) ImageUI : public UIRenderable {
  public:
    static constexpr UUID typeID = "03552ebc7a5d6dea-0aecc877e265992b"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    ImageUI() noexcept;

    void SetTexture(const TextureID &_id) noexcept { m_texture = _id; }
    void SetTint(const Vec4f &_tint) noexcept { m_tint = _tint; }

  protected:
    void Unpack(const ParamView &) override;
    void Collect(RenderList &) override;

  private:
    TextureID m_texture {};
    Vec4f m_tint { 1.f, 1.f, 1.f, 1.f };
    DrawVertex m_vertices[4] {}; //!< rebuilt every Collect; the rect is screen-space and cheap
  };

} // namespace ROSE
