/**

  @file      assetmeta.cpp
  @brief     AssetMeta serialization implementation
  @details   ~
  @author    Viola Case
  @date      19.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/Core/io/ROSE_assetmeta.h>
#include <fstream>
#include <stdexcept>

namespace ROSE {
  Json AssetMeta::ToJson() const {
    return {
      {"rose_version", "0.1"},
      {"uuid",         JsonFromUUID(uuid)},
      {"name",         name},
      {"type",         type},
      {"path",         path},
      {"properties",   properties}
    };
  }

  AssetMeta AssetMeta::FromJson(const Json &j) {
    AssetMeta meta;
    meta.uuid       = JsonToUUID(j.at("uuid"));
    meta.name       = j.at("name").get<std::string>();
    meta.type       = j.at("type").get<std::string>();
    meta.path       = j.at("path").get<std::string>();
    meta.properties = j.value("properties", Json{});
    return meta;
  }

  AssetMeta AssetMeta::LoadFromFile(const std::string &filePath) {
    std::ifstream file(filePath);
    if (!file.is_open())
      throw std::runtime_error("AssetMeta: cannot open '" + filePath + "'");
    return FromJson(Json::parse(file));
  }

  void AssetMeta::SaveToFile(const std::string &filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open())
      throw std::runtime_error("AssetMeta: cannot write '" + filePath + "'");
    file << ToJson().dump(2);
  }
}
