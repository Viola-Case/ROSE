/**

  @file      sceneio.cpp
  @brief     BehaviorFactory registry and SceneIO implementation
  @details   ~
  @author    Viola Case
  @date      21.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/ROSE.h>
#include <ROSE/Core/io/ROSE_sceneio.h>
#include <ROSE/Core/io/ROSE_serializable.h>

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace ROSE {

  // =========================================================================
  // BehaviorFactory
  // =========================================================================

  using RegistryMap = std::unordered_map<std::string, BehaviorFactory::FactoryFn>;

  static RegistryMap &registry() noexcept {
    static RegistryMap reg;
    return reg;
  }

  void BehaviorFactory::Register(const char *typeName, FactoryFn factory) {
    registry()[typeName] = factory;
  }

  UniquePtr<Behavior> BehaviorFactory::Create(const char *typeName) {
    auto it = registry().find(typeName);
    if (it == registry().end())
      throw std::runtime_error(std::string("BehaviorFactory: unknown type '") + typeName + "'");
    return it->second();
  }

  bool BehaviorFactory::IsRegistered(const char *typeName) noexcept {
    return registry().count(typeName) > 0;
  }

  // =========================================================================
  // SceneIO
  // =========================================================================

  SceneDesc SceneIO::Save(const Scene &scene) {
    SceneDesc desc;
    desc.name = String("Scene");

    for (auto &pair : scene.m_objects) {
      const Object &obj = *pair.second;

      ObjectDesc odesc;
      odesc.name      = obj.m_name;
      odesc.uuid      = obj.m_uuid;
      odesc.transform = obj.m_transform;

      for (auto &bpair : obj.m_behaviors) {
        const Behavior         *b   = bpair.second.get();
        const IJsonSerializable *ser = dynamic_cast<const IJsonSerializable *>(b);

        BehaviorDesc bdesc;
        if (ser) {
          bdesc.type       = String(ser->TypeName());
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
      auto obj    = MakeUnique<Object>(odesc.name.c_str(), odesc.transform);
      obj->m_name = odesc.name;
      obj->m_uuid = odesc.uuid;

      for (const BehaviorDesc &bdesc : odesc.behaviors) {
        if (bdesc.type.empty() || !BehaviorFactory::IsRegistered(bdesc.type.c_str()))
          continue;
        auto  behavior = BehaviorFactory::Create(bdesc.type.c_str());
        auto *ser      = dynamic_cast<IJsonSerializable *>(behavior.get());
        if (ser && !bdesc.properties.IsNull())
          ser->FromJson(bdesc.properties);
        obj->AddBehavior(Move(behavior));
      }

      scene.m_objects.insert(odesc.uuid, Move(obj));
    }
  }

} // namespace ROSE
