/**

  @file      ROSE_scenedesc.h
  @brief     Plain-data descriptors for JSON-driven scene setup
  @details   SceneDesc / ObjectDesc / BehaviorDesc are the intermediate
             representation between a .scene file on disk and live
             Scene / Object instances.  Use SceneIO to convert between them.
  @author    Viola Case
  @date      21.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/io/ROSE_json.h>

namespace ROSE {

  struct BehaviorDesc {
    String    type;
    JsonValue properties{};

    [[nodiscard]] JsonValue              ToJson() const;
    [[nodiscard]] static BehaviorDesc    FromJson(const JsonValue &j);
  };

  struct ObjectDesc {
    String             name;
    UUID               uuid{};
    Transform          transform{};
    List<BehaviorDesc> behaviors;

    [[nodiscard]] JsonValue             ToJson() const;
    [[nodiscard]] static ObjectDesc     FromJson(const JsonValue &j);
  };

  struct SceneDesc {
    String            name;
    List<ObjectDesc>  objects;

    [[nodiscard]] JsonValue            ToJson() const;
    [[nodiscard]] static SceneDesc     FromJson(const JsonValue &j);

    [[nodiscard]] static SceneDesc     LoadFromFile(const char *filePath);
    void                               SaveToFile(const char *filePath) const;
  };

} // namespace ROSE
