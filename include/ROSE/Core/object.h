/**

    @file      object.h
    @brief
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/scene.h>
#include <ROSE/Core/rtl.h>
#include <ROSE/Core/uuid.h>
#include <ROSE/Core/transform.h>
#include <ROSE/Core/application.h>

namespace ROSE {
  class Object final {
    friend class Behavior;
    friend class Scene;

  public:
    Object();
    explicit Object(const char *);
    Object(const char *, const Transform &);
    Object(const char *, const Transform &, List<UniquePtr<Behavior>> &&);



    // template<BehaviorType T>
    // T *GetBehaviorOfType() {
    //   auto it = m_behaviors.find(TypeIdOf<T>());
    //   if (it != m_behaviors.end())
    //     return static_cast<T *>(it->second.get());
    //   return nullptr;
    // }
    template <class B>
      requires std::is_base_of_v<Behavior, B>
    Behavior *CreateBehavior() {
      constexpr auto tID = B::TypeID();
      if (auto it = m_behaviors.find(tID); it != m_behaviors.end())
        // return nullptr;
        return m_behaviors.find(tID)->second.get();
      auto b = BehaviorFactory::Create(tID);
      m_behaviors.insert(tID, Move(b));
      return m_behaviors.find(tID)->second.get();
    }

    template <class B>
      requires std::is_base_of_v<Behavior, B>
    Behavior *FindBehavior() noexcept {
      constexpr auto tID = B::TypeID();
      if (auto it = m_behaviors.find(tID); it != m_behaviors.end())
        return it->second.get();
      return nullptr;
    }

    Scene &GetScene() const noexcept;
  private:
    void OnStart() noexcept;
    void FrameUpdate() noexcept;

    void AddBehavior(UniquePtr<Behavior> &&behavior);

    template<BehaviorType B>
    void DestroyBehavior() {
      constexpr auto tID = B::TypeID();
      m_pendingDestroy.push_back(tID);
    }

    Object *GetParent() const noexcept;

    Behavior *GetBehavior(const UUID &) noexcept;



  private:
    UUID m_uuid {};
    String m_name { "Object" };
    TypedHashMap<UUID, UniquePtr<Behavior>> m_behaviors {};
    List<UniquePtr<Behavior>> m_pendingAdd {};
    List<UUID> m_pendingDestroy {};

    Scene *m_scene { nullptr };
    Object *m_parent { nullptr };
  public:
    Transform transform {
      {0,0,0},
      {1,0,0,0}
    };
  };
} // namespace ROSE
