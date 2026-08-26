/**

    @file      behavior.h
    @brief
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/api.h>
#include <ROSE/Core/uuid.h>
#include <ROSE/Core/memory.h>

#include <concepts>

namespace ROSE {
  class Object;

  class ParamView;
  /*!
   * The Object-Behavior Model has three phases, Create, Start, and Update, in that order.
   * - Create phase - behavior is created either at the start of a scene or by another behavior's process.
   * - Start phase - behavior has the opportunity to survey its surroundings and see who else is around. This is where
   * it will reach out to get IDs of its neighbors.
   * - Update phase - behavior performs its tasks as part of the game loop.
   *
   * `OnCreate` should never reach out to other behaviors in the scene.
   * `OnStart` may reach out to other behaviors
   *
   * To summarize,
   * Phase one: touch only yourself. Phase two: reach across and touch your neighbors. (heh)
   *
   * @todo Add behavior name with registry
   */
  class ROSE_API(CORE) Behavior {
    friend class Object;
    friend class Scene;

  protected:
    /*!
     * Called immediately after behavior is added to the object
     */
    virtual void OnCreate() {}
    /*!
     * Called just before first update
     */
    virtual void OnStart();
    /*!
     * Called once every frame
     */
    virtual void FrameUpdate();

    /*!
     * How the object gets unpacked from scene JSON data
     */
    virtual void Unpack(const ParamView &view);

    /*!
     * @todo Never called. There is no SetEnabled()/enable path at all, so nothing can
     * flip m_enabled and nothing fires this. UI (gui.h) already overrides it as dead code.
     */
    virtual void OnEnable();

    /*!
     * @todo Never called - same as OnEnable. Should also fire on teardown, which is
     * currently silent (Scene::FrameUpdate just erases the map entry).
     */
    virtual void OnDisable();

    /*!
     * @todo There is no OnDestroy hook. A behavior that cached neighbor pointers in
     * OnStart has no way to learn they went away.
     */
    virtual void OnDestroy();

    void Enable();
    void Disable();
    bool IsEnabled() const noexcept;

  public:
    virtual UUID GetTypeID() const noexcept = 0;

    virtual ~Behavior();

    // Scene &GetScene() noexcept;
    Object &GetObject() const noexcept;

  protected:
    /* TODO never assigned by anything - default constructed and left that way. Behaviors
     * are keyed by GetTypeID() in Object::m_behaviors, so either give this a real instance
     * identity at install time or drop it. */
    UUID m_uuid;
    Object *m_object { nullptr };

    /* TODO never read. Nothing gates OnStart/FrameUpdate on it and there's no setter, so
     * enabling/disabling a behavior isn't actually expressible yet. See OnEnable/OnDisable. */
    bool m_enabled { true };
  };

  /**
   * Behaviors are declared like:
   * @code{.cpp}
   * class Behavior1 : public Behavior {
   * public:
   *   static constexpr UUID TypeID() { return "ba12c4ae50659b9a-91cc2a6057b9e054"_uuid; }
   *   UUID GetTypeID() const noexcept override { return TypeID(); }
   * }
   * @endcode
   * Remember to generate a new UUID everytime you make a new behavior. Behaviors satisfying
   * RegistrableBehavior can then be passed into your plugin's factory registry via MakeBehavior<T>.
   */
  template <class T>
  concept RegistrableBehavior = std::derived_from<T, Behavior> && std::default_initializable<T> && requires {
    { T::TypeID() } -> std::same_as<UUID>;
  };

  using Behaviour = Behavior;

  using BehaviorReference = UniquePtr<Behavior> *;

} // namespace ROSE