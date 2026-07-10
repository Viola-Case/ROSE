/**

  @file      scene.cpp
  @brief
  @details   ~
  @author    Viola Case
  @date      08.04.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/


#include <ROSE/ROSE.h>
#include <nlohmann/json.hpp>

using JSON = nlohmann::json;


namespace ROSE {
  Application &Scene::GetApplication() const noexcept { return *m_application; }

  void Scene::Bind(Application &app) noexcept {
    m_application = &app;
    for (auto &o : m_objects) o.second->m_scene = this;
    for (auto &o : m_pendingAdd) o->m_scene = this;
  }

  Scene::Scene() {
    static size_t sceneCounter = 1;
    m_name = String(std::format("Scene{}", sceneCounter++).c_str());
  }

  void Scene::FrameUpdate() noexcept {

    if (m_objects.empty()) {

    }
    for (auto &o : m_objects) {
      o.second->FrameUpdate();
    }

    for (const UUID &u : m_pendingDestroy) {
      m_objects.erase(u);
    }

    for (UniquePtr<Object> &o : m_pendingAdd) {
      const auto &u = UUID::Generate();
      o->m_uuid = u;
      o->m_scene = this;
      m_objects.insert(u, Move(o));
    }
    m_pendingAdd.clear();

    for (auto &o : m_objects) {
      for (UniquePtr<Behavior> &b : o.second->m_pendingAdd) {
        b->m_object = o.second.get();
        o.second->m_behaviors.insert(UUID::Generate(), Move(b));
      }
      o.second->m_pendingAdd.clear();
    }
  }

 /*!
  * Put something here to specify we need to talk about JSON
  * ```
{
  "name": "scene1"
  "objects": [
    {
      "uuid": "aaaaaaaaaaaaaaaa-bbbbbbbbbbbbbbbb",
      "name": "Scene Manager",
      "transform": {
        "position": [
          0,0,0
        ],
        "rotation": [
          0,0,0
        ],
        "scale": [
          1,1,1
        ]
      },
      "behaviors": [
        {
          "typeid": "bcebebababebaaaa-bbbbbbbbbbbbbbbb",
          "factoryParameters": [
            "this is a piece of text",
            32,
            true
          ]
        }
      ]
    },
    {
      "uuid": "aaaaaaaaaaaaaaaa-bbbbbbbbbbbbbbbc",
      "name": "Camera",
      "transform": {
        "position": [
          0,0,0
        ],
        "rotation": [
          0,0,0
        ],
        "scale": [
          1,1,1
        ]
      },
      "behaviors": [
        {
          "typeid": "cabebaaaaaaaaaaa-bbbbbbbbbbbbbbbb",
          "uuid": "cabebaaaaaaaaaaa-bbbbbbbbbbbbbbbb"
        }
      ]
    },
    {
      "uuid": "aaaaaaaaaaaaaaaa-bbbbbbbbbbbbbbbd",
      "name": "object3",
      "transform": {
        "position": [
          0,0,0
        ],
        "rotation": [
          0,0,0
        ],
        "scale": [
          1,1,1
        ]
      },
      "behaviors": []
    }
  ]
}
  *
  *
  * @todo Behavior rehydration
  * the comment block below was written by opus 4.8 because it is 4:13 AM and I need to go beddy bye
  *
  * 1. Base Behavior declares the contract:
  *      virtual void Deserialize(const ParamView& params) = 0;  // or non-pure with empty default
  *    - Takes a READ-ONLY view of the "parameters" object, NOT a json& (json is STL-flavored,
  *      keep it Core-side; pass your own thin accessor so the boundary stays clean).
  *    - Object3 has empty params -> a behavior that overrides nothing just no-ops. Free.
  *
  * 2. Loader loop (per behavior entry in the array):
  *      a. Read "id" string -> parse hex -> behavior-type UUID.
  *      b. Ask BehaviorFactory to mint a blank instance by that UUID.
  *         - Factory miss (unknown id) -> log loudly, skip this behavior, keep loading. Limp forward.
  *      c. Hand the minted instance its "parameters" sub-object: instance->Deserialize(params).
  *      d. Move the UniquePtr<Behavior> into the object's behavior map.
  *
  * 3. Each subclass overrides Deserialize and reads ITS OWN named fields:
  *      void Camera::Deserialize(const ParamView& p) {
  *          m_fov     = p.value("fov", 60.0);      // .value with defaults = forward-compat
  *          m_enabled = p.value("enabled", true);  // old files missing a key still load
  *      }
  *    - The base never knows what "fov" is. That's the point. Knowledge lives where the type lives.
  *
  * 4. Open questions to resolve as you go:
  *    - Does Deserialize run BEFORE or AFTER the behavior is attached to its Object?
  *      (Some params may need the owning object to exist. Pick one, document it.)
  *    - "parameters" key missing entirely (not just empty) -> Deserialize gets an empty view,
  *      every .value() falls to default. Make sure ParamView handles absent gracefully.
  */

  inline UUID getUUIDFromNode(const nlohmann::basic_json<> &node) {
    #if UUID_THROW_ON_FAILURE
    #define FALLBACK() throw
    #else
    #define FALLBACK() return UUID::Invalid()
    #endif
    auto s = String(node.get_ref<const std::string&>().c_str());
    if (s.size() != ROSE_UUID_HIGH_LEN + ROSE_UUID_SEPARATOR_LEN + ROSE_UUID_LOW_LEN) FALLBACK();
    // todo check for corrupted bytes in uuid
    const uint128_t high = strtoull(s.c_str(), nullptr, 16);
    const uint64_t low  = strtoull(s.c_str() + ROSE_UUID_HIGH_LEN + ROSE_UUID_SEPARATOR_LEN, nullptr, 16);
    return { (high << 64) | low };
    #undef FALLBACK
  }

  inline Vec3d getVec3FromNode(const nlohmann::basic_json<> &node) {
    Vec3d vec;
    if (!node[0].is_number() || !node[1].is_number() || !node[2].is_number()) return Vec3d{0};
    vec.x = node[0].get<double>();
    vec.y = node[1].get<double>();
    vec.z = node[2].get<double>();
    return vec;
  }

  /*!
   *
   * @param jsonString
   * @return
   */
  Scene Scene::FromJSONString(const String &jsonString) noexcept {
    Scene scene{};

    try {
      JSON json = JSON::parse(jsonString);
      scene.m_name = String(json.at("name").get<std::string>().c_str());

      for (const auto &o : json.at("objects")) {
        /*{
            "uuid": "aaaaaaaaaaaaaaaa-bbbbbbbbbbbbbbbb",
            "name": "Scene Manager",
            "transform": {
              "position": [
                0,0,0
              ],
              "rotation": [
                0,0,0
              ],
              "scale": [
                1,1,1
              ]
            },
            "behaviors": [
              {
                "typeid": "bcebebababebaaaa-bbbbbbbbbbbbbbbb",
                "factoryParameters": [
                  "this is a piece of text",
                  32,
                  true
                ]
              }
            ]
          }*/
        Pair<UUID, UniquePtr<Object>> pair;
        pair.first = getUUIDFromNode(o.at("uuid"));
        pair.second = MakeUnique<Object>();
        Object &obj = *pair.second;
        obj.m_name = String(o.at("name").get<std::string>().c_str());
        const auto &to = o.at("transform");
        Transform &t = obj.m_transform;
        t.position = getVec3FromNode(to.at("position"));
        t.rotation = Quatd::FromEuler(getVec3FromNode(to.at("rotation")));
        t.scale = getVec3FromNode(to.at("scale"));

        for (const auto &b : o.at("behaviors")) {
          UUID tID = getUUIDFromNode(b.at("typeid"));
          auto bvr = BehaviorFactory::Create(tID);
          bvr->m_object = &obj;
          auto paramObj = b.at("factoryParameters");
          ParamView pview{&paramObj};
          bvr->UnpackParameters(pview);
          obj.m_behaviors.insert(tID, Move(bvr));
        }

        scene.m_objects.insert(pair.first, Move(pair.second));
      }
    } catch (nlohmann::json::exception &e) {
      Log(LogLevel::Error, "Scene string corrupt:\n\t{}", e.what());
      return scene;
    }
    return scene;
  }




  String Scene::ToJSONString() noexcept {
    JSON json = { { "name", m_name } };
    for (auto &o : m_objects) {
      struct {
        UUID id;
        String name;
      } pair = { o.first, o.second->m_name };
    }
    return json.dump(2);
  }

  void Scene::AddObject(Object &&obj) noexcept {
    UniquePtr<Object> o(MakeUnique<Object>(Move(obj)));
    m_pendingAdd.push_back(Move(o));
  }

  void Scene::DestroyObject(const UUID &u) noexcept { m_pendingDestroy.push_back(u); }

  Object *Scene::GetObject(const UUID &uuid) noexcept {
    auto it = m_objects.find(uuid);
    if (it != m_objects.end()) return it->second.get();
    return nullptr;
  }
} // namespace ROSE
