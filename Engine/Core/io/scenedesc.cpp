/**

  @file      scenedesc.cpp
  @brief     Scene descriptor serialization implementation
  @details   ~
  @author    Viola Case
  @date      21.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/Core/io/ROSE_scenedesc.h>

namespace ROSE {

  // -------------------------------------------------------------------------
  // BehaviorDesc
  // -------------------------------------------------------------------------

  JsonValue BehaviorDesc::ToJson() const {
    JsonValue j = JsonValue::MakeObject();
    j.Set("type", JsonValue(type.c_str()));
    if (!properties.IsNull())
      j.Set("properties", properties);
    return j;
  }

  BehaviorDesc BehaviorDesc::FromJson(const JsonValue &j) {
    BehaviorDesc desc;
    desc.type = j.At("type").GetString();
    if (j.Contains("properties"))
      desc.properties = j.At("properties");
    return desc;
  }

  // -------------------------------------------------------------------------
  // ObjectDesc
  // -------------------------------------------------------------------------

  JsonValue ObjectDesc::ToJson() const {
    JsonValue jBehaviors = JsonValue::MakeArray();
    for (size_t i = 0; i < behaviors.size(); ++i)
      jBehaviors.Push(behaviors[i].ToJson());

    JsonValue j = JsonValue::MakeObject();
    j.Set("name",      JsonValue(name.c_str()));
    j.Set("uuid",      JsonFromUUID(uuid));
    j.Set("transform", JsonFromTransform(transform));
    j.Set("behaviors", Move(jBehaviors));
    return j;
  }

  ObjectDesc ObjectDesc::FromJson(const JsonValue &j) {
    ObjectDesc desc;
    desc.name      = j.At("name").GetString();
    desc.uuid      = JsonToUUID(j.At("uuid"));
    desc.transform = JsonToTransform(j.At("transform"));
    if (j.Contains("behaviors")) {
      const auto &arr = j.At("behaviors").GetArray();
      for (size_t i = 0; i < arr.size(); ++i)
        desc.behaviors.push_back(BehaviorDesc::FromJson(arr[i]));
    }
    return desc;
  }

  // -------------------------------------------------------------------------
  // SceneDesc
  // -------------------------------------------------------------------------

  JsonValue SceneDesc::ToJson() const {
    JsonValue jObjects = JsonValue::MakeArray();
    for (size_t i = 0; i < objects.size(); ++i)
      jObjects.Push(objects[i].ToJson());

    JsonValue j = JsonValue::MakeObject();
    j.Set("rose_version", JsonValue("0.1"));
    j.Set("name",         JsonValue(name.c_str()));
    j.Set("objects",      Move(jObjects));
    return j;
  }

  SceneDesc SceneDesc::FromJson(const JsonValue &j) {
    SceneDesc desc;
    desc.name = j.At("name").GetString();
    if (j.Contains("objects")) {
      const auto &arr = j.At("objects").GetArray();
      for (size_t i = 0; i < arr.size(); ++i)
        desc.objects.push_back(ObjectDesc::FromJson(arr[i]));
    }
    return desc;
  }

  SceneDesc SceneDesc::LoadFromFile(const char *filePath) {
    return FromJson(JsonValue::ParseFile(filePath));
  }

  void SceneDesc::SaveToFile(const char *filePath) const {
    ToJson().SaveFile(filePath);
  }

} // namespace ROSE
