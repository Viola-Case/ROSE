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


  UniquePtr<Behavior> BehaviorFactory::Create(const UUID &id) noexcept {
    if (!IsRegistered(id)) return nullptr;
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
      if (m == factory.m_behaviorLegend.end()) {}
      Log(LogLevel::Warn, "Duplicate function from {}\n\ttypeid = {}-{}", moduleName, id.high, id.low);
      return RegisterResult::DuplicateID;
    }
    return RegisterResult::Success;
  }

  bool BehaviorFactory::IsRegistered(const UUID &id) noexcept {
    auto it = get().m_factoryFunctions.find(id);
    if (it == get().m_factoryFunctions.end()) {
      return false;
    }
    return true;
  }
} // namespace ROSE


using namespace ROSE;
extern "C" void RoseRegisterCoreModule(BehaviorFactory &factory) {
  /*!
   * If I rearrange BehaviorFactory this part fucks up. Need to come up with a better way without making it accessible
   * anywhere.
   *
   * PSA don't do this
   *
   * @todo This reinterpret_casts the factory to its first member to reach the module list.
   * Reordering BehaviorFactory's members silently corrupts memory with no diagnostic.
   * Give it a real RegisterModule(name) entry point instead.
   */
  List<String> &l = *reinterpret_cast<List<String> *>(&factory);
  List<Pair<FactoryFn, UUID>> fns {
    { MakeBehavior<Camera>, Camera::TypeID() },
    { MakeBehavior<AudioSource>, AudioSource::TypeID() },
    { MakeBehavior<Renderable>, Renderable::TypeID() },
    { MakeBehavior<Motion>, Motion::TypeID() },
  };
  for (const auto &p : fns) {
    switch (factory.Register(p.first, p.second, "Core")) {
    case RegisterResult::Success: case RegisterResult::DuplicateID:
      break;
    case RegisterResult::Failure:
      Log(LogLevel::Error, "Failed to register Core module behavior\n\ttypeid = {}-{}",p.second.high, p.second.low);
      break;
    }
  }
  l.push_back("Core");
}