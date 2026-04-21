/**

  @file      ROSE_assetmeta.h
  @brief     Asset metadata structure for tracking engine assets
  @details   AssetMeta describes one registered asset (texture, sound, mesh…)
             and round-trips through JSON.  File paths are const char* so no
             stdlib string type appears in the public API.
  @author    Viola Case
  @date      21.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/io/ROSE_json.h>

namespace ROSE {

  struct AssetMeta {
    UUID      uuid{};
    String    name;
    String    type;         // "texture" | "sound" | "mesh" | ...
    String    path;         // path relative to assets root
    JsonValue properties{}; // arbitrary key/value metadata

    [[nodiscard]] JsonValue           ToJson() const;
    [[nodiscard]] static AssetMeta    FromJson(const JsonValue &j);

    [[nodiscard]] static AssetMeta    LoadFromFile(const char *filePath);
    void                              SaveToFile(const char *filePath) const;
  };

} // namespace ROSE
