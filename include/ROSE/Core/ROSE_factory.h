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

namespace ROSE {

  using FactoryFn = UniquePtr<Behavior> (*)();

  class BehaviorFactory {
    List<String> RegisteredModules {"Core"};
    TypedHashMap<UUID, FactoryFn> m_factoryFunctions{};
  public:
    void Register(FactoryFn fn) noexcept;
    UniquePtr<Behavior> Create(UUID id) const noexcept;
  };
}