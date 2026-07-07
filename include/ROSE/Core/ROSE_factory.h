/**

  @file       ROSE_factory.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       22.06.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once

#include <ROSE/Core/ROSE_behavior.h>
#include <ROSE/Core/ROSE_hashmap.h>
#include <ROSE/Core/ROSE_list.h>
#include <ROSE/Core/ROSE_memory.h>

namespace ROSE {

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

  class BehaviorFactory {
    List<String> m_registeredModules {"Core"};
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
  };
}

extern "C" void RoseRegisterModule(ROSE::BehaviorFactory&);
