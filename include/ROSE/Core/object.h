/**

    @file      object.h
    @brief
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/api.h>
#include <ROSE/Core/log.h>


#include <ROSE/Core/scene.h>
#include <ROSE/Core/rtl.h>
#include <ROSE/Core/uuid.h>
#include <ROSE/Core/transform.h>
#include <ROSE/Core/application.h>

#include <concepts>

namespace ROSE {
  class ROSE_API(CORE) Object final {
    friend class Behavior;
    friend class Scene;

  public:
    Object();
    explicit Object(const char *);
    Object(const char *, const Transform &);
    Object(const char *, const Transform &, List<UniquePtr<Behavior>> &&);

    /*!
     * @note A competent dev will probably do null checks afterwards. If they don't, they will have a bad time.
     *
     * TODO change name because the "of type" is already implied by the template
     */
    template <BehaviorType T>
    T *GetBehaviorOfType() {
      auto it = m_behaviors.find(T::TypeID());
      if (it != m_behaviors.end()) return static_cast<T *>(it->second.get());
      return nullptr;
    }

    /*!
     * @note This doesn't initialize the behavior upon creation; that happens at the end of the frame during which
     * this function is called. Should probably address this more strongly later.
     */
    template <class B>
      requires std::is_base_of_v<Behavior, B>
    B *CreateBehavior() {
      constexpr auto tID = B::TypeID();
      if (!BehaviorFactory::IsRegistered(tID)) {
        ROSE_LOG_ERROR("Failed to register behavior {} to {}", tID, m_name);
        return nullptr;
      }
      if (auto it = m_behaviors.find(tID); it != m_behaviors.end())
        // return nullptr;
        return m_behaviors.find(tID)->second.get();
      auto b = BehaviorFactory::Create(tID);
      B *ptr = b.get();
      AddBehavior(Move(b));
      return ptr; // TODO disallow interaction until it's fully added
    }

    template <class B>
      requires std::is_base_of_v<Behavior, B>
    B *FindBehavior() noexcept {
      constexpr auto tID = B::TypeID();
      if (auto it = m_behaviors.find(tID); it != m_behaviors.end()) return static_cast<B *>(it->second.get());
      return nullptr;
    }

    Scene &GetScene() const noexcept;

    /*!
     * Visit every behavior on this object. `fn` is called as `fn(Behavior&)`.
     */
    template <typename F>
    requires std::invocable<F, Behavior &>
    void ForEachBehavior(F fn) {
      for (auto &b : m_behaviors)
        fn(*b.second.get());
    }

    const char *GetName() const noexcept;
  private:
    void OnStart() noexcept;
    void FrameUpdate() noexcept;

    void AddBehavior(UniquePtr<Behavior> &&behavior);

    template <BehaviorType B>
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
    /* TODO m_parent is never assigned by anything, so GetParent() always returns nullptr.
     * Either implement the hierarchy (parenting + transform composition) or drop the member. */
    Object *m_parent { nullptr };

  public:
    Transform transform { { 0, 0, 0 }, { 1, 0, 0, 0 } };
  };
} // namespace ROSE
