/**

  @file      ROSE_sceneio.h
  @brief     Scene serialization and behavior factory registry
  @details   BehaviorFactory maps type-name strings to factory functions so
             that SceneIO::Apply can instantiate behaviors from JSON.
             SceneIO::Save captures a live Scene into a SceneDesc.
             SceneIO::Apply populates a Scene from a SceneDesc.
  @author    Viola Case
  @date      19.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <ROSE/Core/io/ROSE_scenedesc.h>
#include <ROSE/Core/rtl/ROSE_memory.h>

namespace ROSE {
  class Scene;
  class Behavior;

  class BehaviorFactory {
  public:
    using FactoryFn = std::function<UniquePtr<Behavior>()>;

    static void Register(const std::string &typeName, FactoryFn factory);

    template<class T>
    static void Register(const std::string &typeName) {
      Register(typeName, []() -> UniquePtr<Behavior> {
        return MakeUnique<T>();
      });
    }

    [[nodiscard]] static UniquePtr<Behavior> Create(const std::string &typeName);
    [[nodiscard]] static bool                IsRegistered(const std::string &typeName) noexcept;

  private:
    [[nodiscard]] static std::unordered_map<std::string, FactoryFn> &Registry() noexcept;
  };

  class SceneIO {
  public:
    [[nodiscard]] static SceneDesc Save(const Scene &scene);
    static void                    Apply(Scene &scene, const SceneDesc &desc);
  };
}
