/**

  @file      sceneio.cpp
  @brief     SceneIO and BehaviorFactory implementation
  @details   ~
  @author    Viola Case
  @date      19.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/ROSE.h>
#include <ROSE/Core/io/ROSE_sceneio.h>
#include <ROSE/Core/io/ROSE_serializable.h>
#include <stdexcept>

namespace ROSE {
  // BehaviorFactory

  std::unordered_map<std::string, BehaviorFactory::FactoryFn> &BehaviorFactory::Registry() noexcept {
    static std::unordered_map<std::string, FactoryFn> reg;
    return reg;
  }

  void BehaviorFactory::Register(const std::string &typeName, FactoryFn factory) {
    Registry()[typeName] = std::move(factory);
  }

  UniquePtr<Behavior> BehaviorFactory::Create(const std::string &typeName) {
    auto it = Registry().find(typeName);
    if (it == Registry().end())
      throw std::runtime_error("BehaviorFactory: unknown type '" + typeName + "'");
    return it->second();
  }

  bool BehaviorFactory::IsRegistered(const std::string &typeName) noexcept {
    return Registry().count(typeName) > 0;
  }

  // SceneIO

  SceneDesc SceneIO::Save(const Scene &scene) {
    SceneDesc desc;
    desc.name = "Scene";

    for (auto &pair : scene.m_objects) {
      const Object &obj = *pair.second;

      ObjectDesc odesc;
      odesc.name      = obj.m_name.c_str();
      odesc.uuid      = obj.m_uuid;
      odesc.transform = obj.m_transform;

      for (auto &bpair : obj.m_behaviors) {
        const Behavior        *b    = bpair.second.get();
        const IJsonSerializable *ser = dynamic_cast<const IJsonSerializable *>(b);

        BehaviorDesc bdesc;
        if (ser) {
          bdesc.type       = ser->TypeName();
          bdesc.properties = ser->ToJson();
        }
        odesc.behaviors.push_back(Move(bdesc));
      }

      desc.objects.push_back(Move(odesc));
    }
    return desc;
  }

  void SceneIO::Apply(Scene &scene, const SceneDesc &desc) {
    for (const ObjectDesc &odesc : desc.objects) {
      auto obj       = MakeUnique<Object>(odesc.name.c_str(), odesc.transform);
      obj->m_name    = String(odesc.name.c_str());
      obj->m_uuid    = odesc.uuid;

      for (const BehaviorDesc &bdesc : odesc.behaviors) {
        if (bdesc.type.empty() || !BehaviorFactory::IsRegistered(bdesc.type))
          continue;

        auto behavior = BehaviorFactory::Create(bdesc.type);
        auto *ser     = dynamic_cast<IJsonSerializable *>(behavior.get());
        if (ser && !bdesc.properties.is_null())
          ser->FromJson(bdesc.properties);

        obj->AddBehavior(Move(behavior));
      }

      scene.m_objects.insert(odesc.uuid, Move(obj));
    }
  }
}
