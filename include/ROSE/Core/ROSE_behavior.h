/**

    @file      ROSE_behavior.h
    @brief
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

namespace ROSE {
  class Behavior {
    friend class Object;
    friend class Scene;

  protected:
    virtual void OnStart() = 0;
    virtual void FrameUpdate() = 0;
    virtual void FixedUpdate() = 0;
    void SetObject() const noexcept;

  public:
    virtual ~Behavior() = default;

    Scene &GetScene() noexcept;
    Object &GetObject() noexcept;

  protected:
    UUID m_uuid;
    Object *m_object;
  };

  using Behaviour = Behavior;
} // namespace ROSE