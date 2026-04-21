/**

  @file      ROSE_sceneio.h
  @brief     Scene serialization and behavior factory registry
  @details   BehaviorFactory maps type-name strings to zero-argument factory
             functions so that SceneIO::Apply can instantiate behaviors from a
             SceneDesc loaded out of JSON.

             Usage:
               BehaviorFactory::Register<MyBehavior>("MyBehavior");
               SceneDesc desc = SceneDesc::LoadFromFile("level1.scene");
               SceneIO::Apply(scene, desc);

  @author    Viola Case
  @date      21.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/io/ROSE_scenedesc.h>
#include <ROSE/Core/rtl/ROSE_memory.h>

namespace ROSE {

  class Scene;
  class Behavior;

  class BehaviorFactory {
  public:
    using FactoryFn = UniquePtr<Behavior>(*)();

    static void Register(const char *typeName, FactoryFn factory);

    template<class T>
    static void Register(const char *typeName) {
      Register(typeName, []() -> UniquePtr<Behavior> { return MakeUnique<T>(); });
    }

    [[nodiscard]] static UniquePtr<Behavior> Create(const char *typeName);
    [[nodiscard]] static bool                IsRegistered(const char *typeName) noexcept;
  };

  class SceneIO {
  public:
    [[nodiscard]] static SceneDesc Save(const Scene &scene);
    static void                    Apply(Scene &scene, const SceneDesc &desc);
  };

} // namespace ROSE
