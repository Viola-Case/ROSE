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

  /**
    @class   BehaviorFactory
    @brief   Static registry mapping type-name strings to behavior factory functions.
    @details Register every concrete Behavior subclass before calling
             SceneIO::Apply so that behavior names in a .scene file can be
             resolved to live instances.
  **/
  class BehaviorFactory {
  public:
    using FactoryFn = UniquePtr<Behavior>(*)();

    /**
      @brief   Registers a raw factory function under the given type name.
      @param   typeName  Unique string identifier for the behavior type.
      @param   factory   Zero-argument function that allocates a new instance.
    **/
    static void Register(const char *typeName, FactoryFn factory);

    /**
      @brief   Convenience overload: registers a default-constructible behavior type.
      @tparam  T         Concrete Behavior subclass to register.
      @param   typeName  Unique string identifier for the type.
    **/
    template<class T>
    static void Register(const char *typeName) {
      Register(typeName, []() -> UniquePtr<Behavior> { return MakeUnique<T>(); });
    }

    /**
      @brief   Creates a new behavior instance by type name.
      @param   typeName  Name previously passed to Register().
      @retval  Owning pointer to the new instance.
      @note    Asserts if typeName has not been registered.
    **/
    [[nodiscard]] static UniquePtr<Behavior> Create(const char *typeName);

    /**
      @brief   Returns true if typeName has been registered.
      @param   typeName  Name to check.
    **/
    [[nodiscard]] static bool                IsRegistered(const char *typeName) noexcept;
  };

  /**
    @class   SceneIO
    @brief   Converts between live Scene objects and serialisable SceneDesc snapshots.
  **/
  class SceneIO {
  public:
    /**
      @brief   Captures the current state of a scene into a plain-data SceneDesc.
      @param   scene  Live scene to snapshot.
      @retval  SceneDesc representing the scene's current objects and behaviors.
               Behaviors that implement IJsonSerializable have their properties captured.
    **/
    [[nodiscard]] static SceneDesc Save(Scene &scene);

    /**
      @brief   Populates a scene from a SceneDesc, creating and initialising objects.
      @param   scene  Target scene to populate (existing objects are replaced).
      @param   desc   SceneDesc loaded from a .scene file or built programmatically.
               Behavior types must be registered with BehaviorFactory before calling.
    **/
    static void                    Apply(Scene &scene, const SceneDesc &desc);
  };

} // namespace ROSE
