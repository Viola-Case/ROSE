/**

  @file       factory.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       22.06.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once

#include <ROSE/Core/behavior.h>
#include <ROSE/Core/hashmap.h>
#include <ROSE/Core/list.h>
#include <ROSE/Core/memory.h>

namespace ROSE {
  class BehaviorFactory;

  enum class RegisterResult {
    Success,
    DuplicateID,
    Failure
  };

  //enum class DuplicateResolution {
  //  UseNewer,
  //  UseOlder,
  //  UseMine,
  //  UseTheirs
  //};

  using FactoryFn = UniquePtr<Behavior> (*)();

  /*!
   * Canonical FactoryFn for a behavior type; register as `MakeBehavior<MyBehavior>`.
   * The factory builds a blank object, then Deserialize() fills it.
   */
  template <RegistrableBehavior T>
  UniquePtr<Behavior> MakeBehavior() { return MakeUnique<T>(); }

  class FactoryRegistrar {
    FactoryRegistrar(BehaviorFactory &factory);
  };

  class CoreFactoryRegistrar : public FactoryRegistrar {
  public:
    void FinalizeModuleRegistration();
  };

  /*!
   *  
   */
  class BehaviorFactory {
  friend class FactoryRegistrar;
    List<String> m_registeredModules {};
    TypedHashMap<UUID, FactoryFn> m_factoryFunctions{};
    TypedHashMap<UUID, String> m_behaviorLegend{};
  public:
    static BehaviorFactory &get();
    /*!
     * @param fn pointer to a `UniquePtr<Behavior>()`
     * @param id said behavior's typeid
     * @param moduleName name of the module owning the function
     */
    static RegisterResult Register(FactoryFn fn, const UUID &id, const char *moduleName = "");
    static UniquePtr<Behavior> Create(const UUID &id) noexcept;
    static bool IsRegistered(const UUID &id) noexcept;
  };
}

extern "C" void RoseRegisterCoreModule(ROSE::BehaviorFactory&);
