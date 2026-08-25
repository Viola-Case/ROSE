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

namespace ROSE {
  enum class RenderableType {
    Sprite,
    Mesh,
    UI,
    InstancedMesh,
    Shape
  };

  /*!
   *
   */
  class Renderable : public Behavior {
    friend class RenderBackend;
  public:
    virtual RenderableType GetRenderableType() const noexcept = 0;
  private:
    void *m_renderer = nullptr;
  };

  /*!
   *
   */
  class MeshRenderable : public Renderable {
  public:
    static constexpr UUID typeID = "9cfaac61886d5e2e-9e75d84332659afc"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    static constexpr RenderableType renderableType = RenderableType::Mesh;
    static constexpr RenderableType Type() { return renderableType; }
    RenderableType GetRenderableType() const noexcept override { return Type(); }
  protected:
    //void Unpack(const ParamView &) override;
    Mesh *m_mesh {};
  };

  /*!
   *
   */
  class SpriteRenderable : public Renderable {
  public:
    static constexpr UUID typeID = "32c5294ce2e000fd-271098999265987f"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    static constexpr RenderableType renderableType = RenderableType::Sprite;
    static constexpr RenderableType Type() { return renderableType; }
    RenderableType GetRenderableType() const noexcept override { return Type(); }
  protected:
    //void Unpack(const ParamView &) override;
  };

  class UIRenderable : public Renderable {
  public:
    static constexpr UUID typeID = "cb497ab81ab3dd7b-85272b1b21659b69"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    static constexpr RenderableType renderableType = RenderableType::UI;
    static constexpr RenderableType Type() { return renderableType; }
    RenderableType GetRenderableType() const noexcept override { return Type(); }
  protected:
    //void Unpack(const ParamView &) override;
  };

  class TextUI : public UIRenderable {
  public:
    static constexpr UUID typeID = "9f24e65aa0a93483-992866975b659b59"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    static constexpr RenderableType renderableType = RenderableType::UI;
    static constexpr RenderableType Type() { return renderableType; }
    RenderableType GetRenderableType() const noexcept override { return Type(); }
  protected:
    //void Unpack(const ParamView &) override;
  };

  class ImageUI : public UIRenderable {
  public:
    static constexpr UUID typeID = "03552ebc7a5d6dea-0aecc877e265992b"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    static constexpr RenderableType renderableType = RenderableType::UI;
    static constexpr RenderableType Type() { return renderableType; }
    RenderableType GetRenderableType() const noexcept override { return Type(); }
  protected:
    //void Unpack(const ParamView &) override;
  };

  class ShapeRenderable : public Renderable {
  public:
    static constexpr UUID typeID = "4e503b30bef7e6ef-a5250bb2d5659727"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    static constexpr RenderableType renderableType = RenderableType::UI;
    static constexpr RenderableType Type() { return renderableType; }
    RenderableType GetRenderableType() const noexcept override { return Type(); }
  protected:
    //void Unpack(const ParamView &) override;
  };

}