/**

  @file      ROSE_serializable.h
  @brief     Interface for JSON-serializable behaviors
  @details   Behaviors that implement IJsonSerializable can have their
             state captured by SceneIO::Save and restored via SceneIO::Apply.
  @author    Viola Case
  @date      21.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/io/ROSE_json.h>

namespace ROSE {

  /**
    @class   IJsonSerializable
    @brief   Mixin interface for behaviors whose state can be round-tripped through JSON.
    @details Implement this alongside Behavior to participate in SceneIO::Save /
             SceneIO::Apply. The returned TypeName() must match the name registered
             with BehaviorFactory::Register so that Apply() can recreate the behavior.
  **/
  class IJsonSerializable {
  public:
    virtual ~IJsonSerializable() = default;

    /**
      @brief   Returns the registered type name string for this behavior.
               Must match the name passed to BehaviorFactory::Register<T>().
    **/
    [[nodiscard]] virtual const char *TypeName() const = 0;

    /**
      @brief   Serialises this behavior's state into a JsonValue.
      @retval  JsonValue of object type containing all fields needed by FromJson().
    **/
    [[nodiscard]] virtual JsonValue   ToJson()   const = 0;

    /**
      @brief   Restores this behavior's state from a previously serialised JsonValue.
      @param   j  JsonValue produced by ToJson().
    **/
    virtual void                      FromJson(const JsonValue &j) = 0;
  };

} // namespace ROSE
