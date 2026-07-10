/**

    @file      ROSE_behavior.h
    @brief
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_uuid.h>

#include <concepts>

namespace ROSE {
  class ParamView;

  class Behavior {
    friend class Object;
    friend class Scene;

  protected:
    virtual void OnStart() {}
    virtual void FrameUpdate() {}
    virtual void FixedUpdate() {}

    virtual void Unpack(const ParamView &view) {}

  public:
    virtual UUID GetTypeID() const noexcept = 0;

    virtual ~Behavior() {}

    virtual void UnpackParameters(const ParamView &) {}

    //Scene &GetScene() noexcept;
    Object &GetObject() noexcept;

  protected:
    UUID m_uuid;
    Object *m_object { nullptr };
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
  concept RegistrableBehavior =
      std::derived_from<T, Behavior> &&
      std::default_initializable<T> &&
      requires { { T::TypeID() } -> std::same_as<UUID>; };

  using Behaviour = Behavior;

} // namespace ROSE