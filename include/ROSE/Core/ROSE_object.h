/**

    @file      ROSE_object.h
    @brief     Object class — a named, transformable entity in a scene
    @details   Object is the fundamental game entity. It owns a transform,
               a UUID, and an optional set of Behavior components that drive
               its logic. Objects are created by the Scene and live for the
               duration of the scene unless explicitly destroyed.
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <utility>
#include <ROSE/Core/ROSE_rtl.h>
#include <ROSE/Core/ROSE_uuid.h>
#include <ROSE/Core/ROSE_transform.h>

namespace ROSE {
  class Scene;
  class Behavior;

  /**
    @concept BehaviorType
    @brief   Constrains a type to be a concrete Behavior subclass.
  **/
  template <class T>
  concept BehaviorType = std::derived_from<T, Behavior>;


  /**
    @class   Object
    @brief   A named, transformable game entity that can host Behavior components.
    @details Each object has a UUID, a Transform (position/rotation/scale), a
             display name, and an associated set of Behaviors. Behaviors are
             added and removed lazily — changes take effect at the start of
             the next frame.
  **/
  class Object final {
    friend class Behavior;
    friend class Scene;
    friend class SceneIO;

  public:
    Object();                                                               //!< Default constructor; name is "Object", identity transform
    explicit Object(const char*);                                          //!< Constructs with the given display name and identity transform
    Object(const char*, const Transform &);                               //!< Constructs with a name and an explicit transform
    Object(const char*, const Transform &, List<UniquePtr<Behavior>> &&); //!< Constructs with a name, transform, and initial behavior list



    //template<BehaviorType T>
    //T *GetBehaviorOfType() {
    //  auto it = m_behaviors.find(TypeIdOf<T>());
    //  if (it != m_behaviors.end())
    //    return static_cast<T *>(it->second.get());
    //  return nullptr;
    //}

  private:
    /**
      @brief   Called once when the object first becomes active; starts all pending behaviors.
    **/
    void OnStart() noexcept;

    /**
      @brief   Advances the object by one frame; updates all active behaviors.
    **/
    void FrameUpdate() noexcept;

    /**
      @brief   Takes ownership of a Behavior and queues it for activation next frame.
      @param   behavior  The behavior instance to add.
    **/
    void AddBehavior(UniquePtr<Behavior>&& behavior) noexcept;

    /**
      @brief   Schedules the behavior with the given UUID for removal at end of frame.
      @param   UUID of the behavior to destroy.
    **/
    void DestroyBehavior(const UUID &) noexcept;

    /**
      @brief   Returns a reference to the Scene that owns this object.
    **/
    Scene& GetScene() const noexcept;

    /**
      @brief   Returns a reference to this object's parent, if one exists.
      @warning Behaviour is undefined if this object has no parent.
    **/
    Object& GetParent() const noexcept;

    /**
      @brief   Finds a behavior by its UUID.
      @retval  Pointer to the Behavior, or nullptr if not found.
    **/
    Behavior *GetBehavior(const UUID &) noexcept;



  private:

    String m_name{ "Object" };
    TypedHashMap<UUID, UniquePtr<Behavior>> m_behaviors{};
    List<UniquePtr<Behavior>> m_pendingAdd{};
    List<UUID> m_pendingDestroy{};

    UUID m_uuid{};
    Scene* const m_scene{ nullptr };
    Object* m_parent{ nullptr };
    Transform m_transform{ 0, 1 };
  };
}
