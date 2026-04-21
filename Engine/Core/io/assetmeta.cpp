/**

  @file      assetmeta.cpp
  @brief     AssetMeta serialization implementation
  @details   ~
  @author    Viola Case
  @date      21.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/Core/io/ROSE_assetmeta.h>

namespace ROSE {

  JsonValue AssetMeta::ToJson() const {
    JsonValue j = JsonValue::MakeObject();
    j.Set("rose_version", JsonValue("0.1"));
    j.Set("uuid",         JsonFromUUID(uuid));
    j.Set("name",         JsonValue(name.c_str()));
    j.Set("type",         JsonValue(type.c_str()));
    j.Set("path",         JsonValue(path.c_str()));
    if (!properties.IsNull())
      j.Set("properties", properties);
    return j;
  }

  AssetMeta AssetMeta::FromJson(const JsonValue &j) {
    AssetMeta meta;
    meta.uuid = JsonToUUID(j.At("uuid"));
    meta.name = j.At("name").GetString();
    meta.type = j.At("type").GetString();
    meta.path = j.At("path").GetString();
    if (j.Contains("properties"))
      meta.properties = j.At("properties");
    return meta;
  }

  AssetMeta AssetMeta::LoadFromFile(const char *filePath) {
    return FromJson(JsonValue::ParseFile(filePath));
  }

  void AssetMeta::SaveToFile(const char *filePath) const {
    ToJson().SaveFile(filePath);
  }

} // namespace ROSE
