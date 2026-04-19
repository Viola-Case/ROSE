/**

  @file      ROSE_scenedesc.h
  @brief     Plain-data descriptors for JSON-driven scene setup
  @details   SceneDesc / ObjectDesc / BehaviorDesc form the intermediate
             representation between a JSON file on disk and live Scene/Object
             instances.  Use SceneIO to convert between these descriptors and
             running scenes.
  @author    Viola Case
  @date      19.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <string>
#include <vector>
#include <ROSE/Core/io/ROSE_json.h>

namespace ROSE {
  struct BehaviorDesc {
    std::string type;
    Json        properties{};

    [[nodiscard]] Json                ToJson() const;
    [[nodiscard]] static BehaviorDesc FromJson(const Json &j);
  };

  struct ObjectDesc {
    std::string              name;
    UUID                     uuid{};
    Transform                transform{};
    std::vector<BehaviorDesc> behaviors;

    [[nodiscard]] Json               ToJson() const;
    [[nodiscard]] static ObjectDesc  FromJson(const Json &j);
  };

  struct SceneDesc {
    std::string             name;
    std::vector<ObjectDesc> objects;

    [[nodiscard]] Json              ToJson() const;
    [[nodiscard]] static SceneDesc  FromJson(const Json &j);

    [[nodiscard]] static SceneDesc  LoadFromFile(const std::string &filePath);
    void                            SaveToFile(const std::string &filePath) const;
  };
}
