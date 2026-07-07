/**

  @file       factory.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       22.06.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>
namespace ROSE {
  BehaviorFactory &BehaviorFactory::get() {
    static BehaviorFactory factory;
    return factory;
  }

  UniquePtr<ROSE::Behavior> MakeCamera() { return MakeUnique<Camera>(); }

  UniquePtr<Behavior> BehaviorFactory::Create(const UUID &id) noexcept {
    auto it = get().m_factoryFunctions.find(id);
    if (it == get().m_factoryFunctions.end()) {
      return nullptr;
    }
    return it->second();
  }

  RegisterResult BehaviorFactory::Register(FactoryFn fn, const UUID &id, const char *moduleName) {
    auto &factory = get();
    auto it = factory.m_factoryFunctions.find(id);
    if (it == factory.m_factoryFunctions.end()) {
      auto ins = factory.m_factoryFunctions.insert(id, fn);
      if (ins == factory.m_factoryFunctions.end()) return RegisterResult::Failure;
    } else {
      auto m = factory.m_behaviorLegend.find(id);
      if (m == factory.m_behaviorLegend.end()) {
        
      }
      Log(LogLevel::Warn, "Duplicate function from {}\n\ttypeid = {}-{}", moduleName, id.high, id.low);
      return RegisterResult::DuplicateID;
    }
    return RegisterResult::Success;
  }
} // namespace ROSE

extern "C" void RoseRegisterModule(ROSE::BehaviorFactory &) {
  // ROSE::Camera::
}