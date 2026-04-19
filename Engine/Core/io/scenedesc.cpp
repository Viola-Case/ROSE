/**

  @file      scenedesc.cpp
  @brief     Scene descriptor serialization implementation
  @details   ~
  @author    Viola Case
  @date      19.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/Core/io/ROSE_scenedesc.h>
#include <fstream>
#include <stdexcept>

namespace ROSE {
  // BehaviorDesc

  Json BehaviorDesc::ToJson() const {
    return {{"type", type}, {"properties", properties}};
  }

  BehaviorDesc BehaviorDesc::FromJson(const Json &j) {
    BehaviorDesc desc;
    desc.type       = j.at("type").get<std::string>();
    desc.properties = j.value("properties", Json{});
    return desc;
  }

  // ObjectDesc

  Json ObjectDesc::ToJson() const {
    Json jBehaviors = Json::array();
    for (const auto &b : behaviors)
      jBehaviors.push_back(b.ToJson());

    return {
      {"name",      name},
      {"uuid",      JsonFromUUID(uuid)},
      {"transform", JsonFromTransform(transform)},
      {"behaviors", jBehaviors}
    };
  }

  ObjectDesc ObjectDesc::FromJson(const Json &j) {
    ObjectDesc desc;
    desc.name      = j.at("name").get<std::string>();
    desc.uuid      = JsonToUUID(j.at("uuid"));
    desc.transform = JsonToTransform(j.at("transform"));

    if (j.contains("behaviors")) {
      for (const auto &b : j.at("behaviors"))
        desc.behaviors.push_back(BehaviorDesc::FromJson(b));
    }
    return desc;
  }

  // SceneDesc

  Json SceneDesc::ToJson() const {
    Json jObjects = Json::array();
    for (const auto &o : objects)
      jObjects.push_back(o.ToJson());

    return {
      {"rose_version", "0.1"},
      {"name",         name},
      {"objects",      jObjects}
    };
  }

  SceneDesc SceneDesc::FromJson(const Json &j) {
    SceneDesc desc;
    desc.name = j.at("name").get<std::string>();

    if (j.contains("objects")) {
      for (const auto &o : j.at("objects"))
        desc.objects.push_back(ObjectDesc::FromJson(o));
    }
    return desc;
  }

  SceneDesc SceneDesc::LoadFromFile(const std::string &filePath) {
    std::ifstream file(filePath);
    if (!file.is_open())
      throw std::runtime_error("SceneDesc: cannot open '" + filePath + "'");
    return FromJson(Json::parse(file));
  }

  void SceneDesc::SaveToFile(const std::string &filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open())
      throw std::runtime_error("SceneDesc: cannot write '" + filePath + "'");
    file << ToJson().dump(2);
  }
}
