/**

    @file      ROSE_behavior.h
    @brief     Behavior abstract base class for user-written game logic components
    @details   Subclass Behavior and override OnStart(), FrameUpdate(), and
               FixedUpdate() to implement game logic. Attach instances to an
               Object via SceneIO or other factory mechanisms. Each Behavior
               has access to its owning Object and, through it, the Scene.
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

namespace ROSE {
  /**
    @class   Behavior
    @brief   Abstract base class for game logic components attached to an Object.
    @details Override the three pure virtual callbacks to implement game logic.
             The engine calls OnStart() once on activation and FrameUpdate()
             every frame thereafter. FixedUpdate() will be called on a fixed
             timestep (not yet wired into the scheduler).
  **/
  class Behavior {
    friend class Object;
    friend class Scene;
  protected:
    /**
      @brief   Called once when this behavior is first activated.
               Override to perform one-time setup (e.g. cache component references).
    **/
    virtual void OnStart() = 0;

    /**
      @brief   Called once per frame.
               Override to implement frame-rate-dependent game logic.
    **/
    virtual void FrameUpdate() = 0;

    /**
      @brief   Called on a fixed timestep, suitable for physics.
               Override to implement timestep-stable simulation logic.
    **/
    virtual void FixedUpdate() = 0;

    /**
      @brief   Internal: links this behavior to its parent Object.
    **/
    void SetObject() const noexcept;

  public:
    virtual ~Behavior() = default;

    /**
      @brief   Returns a reference to the Scene that owns this behavior's object.
    **/
    Scene &GetScene() noexcept;

    /**
      @brief   Returns a reference to the Object that owns this behavior.
    **/
    Object &GetObject() noexcept;

  protected:
    UUID m_uuid;
    Object *m_object;



  };

  /// @brief Alias for Behavior using British English spelling.
  using Behaviour = Behavior;
}
