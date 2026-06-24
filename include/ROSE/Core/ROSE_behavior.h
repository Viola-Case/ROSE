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
  class ParamView;

  class Behavior {
    friend class Object;
    friend class Scene;

  protected:
    virtual void OnStart() {};
    virtual void FrameUpdate() {};
    virtual void FixedUpdate() {};
    // void SetObject() const noexcept;

  public:
    virtual constexpr UUID GetTypeID() const noexcept = 0;

    virtual ~Behavior();

    virtual void UnpackParameters(const ParamView &);

    Scene &GetScene() noexcept;
    Object &GetObject() noexcept;

  protected:
    UUID m_uuid;
    Object *m_object;
  };

  /**
   * Behaviors are declared like:
   * ```cpp
   * class Behavior1 : public BehaviorBase<Behavior1> {
   *   static constexpr TypeID() { return 0xba12c4ae50659b9a91cc2a6057b9e054; }
   * }
   * ```
   * Remember to generate a new UUID everytime you make a new behavior. You should then pass the constexpr functions
   * into your plugin's factory registry.
   *
   * @tparam Derived Behavior subclass
   */
  template <class Derived>
  class BehaviorBase : public Behavior {
    static_assert(std::is_same_v<decltype(Derived::TypeID()), UUID>,
                  "Behaviors must define: static constexpr UUID TypeID()");
    static constexpr UUID GetTypeID() noexcept { return Derived::TypeID(); }
    static_assert(std::is_default_constructible_v<Derived>,
                  "Registrable behaviors must be default-constructible: "
                  "the factory builds a blank object, then Deserialize() fills it.");
    static Behavior *Create() { return new Derived; }
  };

  using Behaviour = Behavior;
  template <class Derived>
  using BehaviourBase = BehaviorBase<Derived>;

} // namespace ROSE