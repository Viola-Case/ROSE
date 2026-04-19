/**

  @file      ROSE_assetmeta.h
  @brief     Asset metadata structure for tracking engine assets
  @details   AssetMeta describes a single registered asset (texture, sound,
             mesh, etc.) and can be serialized to / deserialized from JSON.
  @author    Viola Case
  @date      19.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <string>
#include <ROSE/Core/io/ROSE_json.h>

namespace ROSE {
  struct AssetMeta {
    UUID        uuid{};
    std::string name;
    std::string type;        // "texture" | "sound" | "mesh" | ...
    std::string path;        // file path relative to assets root
    Json        properties{};  // arbitrary key/value metadata

    [[nodiscard]] Json             ToJson() const;
    [[nodiscard]] static AssetMeta FromJson(const Json &j);

    [[nodiscard]] static AssetMeta LoadFromFile(const std::string &filePath);
    void                           SaveToFile(const std::string &filePath) const;
  };
}
