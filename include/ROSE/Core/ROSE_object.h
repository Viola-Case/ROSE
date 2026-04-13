/**

    @file      ROSE_object.h
    @brief     
    @details   ~
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
  template <class T>
  concept BehaviorType = std::derived_from<T, Behavior>;


  class Object {
    friend class Behavior;
    friend class Scene;

  public:
    Object();


    Scene &GetScene() const noexcept;

    Object &GetParent() const noexcept;

    TypedHashMap<UUID, UniquePtr<Behavior>> &GetBehaviors() noexcept;

    //template<BehaviorType T>
    //T *GetBehaviorOfType() {
    //  auto it = m_behaviors.find(TypeIdOf<T>());
    //  if (it != m_behaviors.end())
    //    return static_cast<T *>(it->second.get());
    //  return nullptr;
    //}

  private:

    void OnStart() noexcept;
    void FrameUpdate() noexcept;

    String m_name{"Object"};
    TypedHashMap<UUID, UniquePtr<Behavior>> m_behaviors {};
    Scene *const m_scene{ nullptr };
    Object *m_parent{ nullptr };
    Transform m_transform{0,1};
  };
}