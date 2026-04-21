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


  class Object final {
    friend class Behavior;
    friend class Scene;

  public:
    Object();
    explicit Object(const char*);
    Object(const char*, const Transform &);
    Object(const char*, const Transform &, List<UniquePtr<Behavior>> &&);



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

    void AddBehavior(UniquePtr<Behavior>&& behavior) noexcept;
    void DestroyBehavior(const UUID &) noexcept;

    Scene& GetScene() const noexcept;
    Object& GetParent() const noexcept;

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
