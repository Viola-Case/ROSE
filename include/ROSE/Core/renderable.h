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
    static constexpr RenderableType renderableType = RenderableType::Mesh;
    static constexpr RenderableType Type() { return renderableType; }
    RenderableType GetRenderableType() const noexcept override { return Type(); }
  protected:
    void Unpack(const ParamView &) override;
    Mesh *m_mesh {};
  };

  /*!
   *
   */
  class SpriteRenderable : public Renderable {
  public:
    static constexpr RenderableType renderableType = RenderableType::Sprite;
    static constexpr RenderableType Type() { return renderableType; }
    RenderableType GetRenderableType() const noexcept override { return Type(); }
  protected:
    void Unpack(const ParamView &) override;
  };

  class UIRenderable : public Renderable {
  public:
    static constexpr RenderableType renderableType = RenderableType::UI;
    static constexpr RenderableType Type() { return renderableType; }
    RenderableType GetRenderableType() const noexcept override { return Type(); }
  protected:
    void Unpack(const ParamView &) override;
  };

  class TextUI : public UIRenderable {
  public:
    static constexpr RenderableType renderableType = RenderableType::UI;
    static constexpr RenderableType Type() { return renderableType; }
    RenderableType GetRenderableType() const noexcept override { return Type(); }
  protected:
    void Unpack(const ParamView &) override;
  };

  class ImageUI : public UIRenderable {
  public:
    static constexpr RenderableType renderableType = RenderableType::UI;
    static constexpr RenderableType Type() { return renderableType; }
    RenderableType GetRenderableType() const noexcept override { return Type(); }
  protected:
    void Unpack(const ParamView &) override;
  };

  class ShapeRenderable : public Renderable {
  public:
    static constexpr RenderableType renderableType = RenderableType::UI;
    static constexpr RenderableType Type() { return renderableType; }
    RenderableType GetRenderableType() const noexcept override { return Type(); }
  protected:
    void Unpack(const ParamView &) override;
  };

}