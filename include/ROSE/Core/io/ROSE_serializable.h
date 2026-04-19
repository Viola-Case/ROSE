/**

  @file      ROSE_serializable.h
  @brief     Interface for JSON-serializable behaviors
  @details   Behaviors that implement IJsonSerializable can have their state
             captured by SceneIO::Save and restored via SceneIO::Apply.
  @author    Viola Case
  @date      19.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <string>
#include <ROSE/Core/io/ROSE_json.h>

namespace ROSE {
  class IJsonSerializable {
  public:
    virtual ~IJsonSerializable() = default;

    [[nodiscard]] virtual std::string TypeName() const = 0;
    [[nodiscard]] virtual Json        ToJson()   const = 0;
    virtual void                      FromJson(const Json &j) = 0;
  };
}
