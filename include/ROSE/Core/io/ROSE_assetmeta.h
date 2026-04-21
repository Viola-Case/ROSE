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

  /**
    @struct  AssetMeta
    @brief   Metadata record describing one engine asset.
    @details Each asset has a stable UUID, a human-readable name, a type tag,
             a path relative to the assets root, and an open-ended JSON property
             bag for engine-specific or tool-specific attributes.
  **/
  struct AssetMeta {
    UUID      uuid{};         //!< Stable identifier; never changes after initial creation
    String    name;           //!< Human-readable asset name (e.g. "PlayerTexture")
    String    type;           //!< Asset category: "texture" | "sound" | "mesh" | ...
    String    path;           //!< Path relative to the assets root directory
    JsonValue properties{};  //!< Arbitrary key/value metadata for tools or the engine

    /**
      @brief   Serialises this record to a JSON object.
      @retval  JsonValue of object type with "uuid", "name", "type", "path", and "properties".
    **/
    [[nodiscard]] JsonValue           ToJson() const;

    /**
      @brief   Deserialises an AssetMeta from a JSON object.
      @param   j  JsonValue of object type containing the expected fields.
    **/
    [[nodiscard]] static AssetMeta    FromJson(const JsonValue &j);

    /**
      @brief   Loads an AssetMeta from a .asset JSON file on disk.
      @param   filePath  Path to the .asset file.
    **/
    [[nodiscard]] static AssetMeta    LoadFromFile(const char *filePath);

    /**
      @brief   Writes this record to a .asset JSON file on disk.
      @param   filePath  Destination path; parent directory must exist.
    **/
    void                              SaveToFile(const char *filePath) const;
  };

} // namespace ROSE
