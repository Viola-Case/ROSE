/**

  @file      ROSE_scenedesc.h
  @brief     Plain-data descriptors for JSON-driven scene setup
  @details   SceneDesc / ObjectDesc / BehaviorDesc are the intermediate
             representation between a .scene file on disk and live
             Scene / Object instances.  Use SceneIO to convert between them.
  @author    Viola Case
  @date      21.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/io/ROSE_json.h>

namespace ROSE {

  /**
    @struct  BehaviorDesc
    @brief   Plain-data snapshot of one behavior instance.
    @details Stores the registered type name and an arbitrary JSON property bag,
             sufficient to recreate the behavior via BehaviorFactory and
             IJsonSerializable::FromJson().
  **/
  struct BehaviorDesc {
    String    type;           //!< Registered type name used by BehaviorFactory::Create()
    JsonValue properties{};  //!< Behavior-specific serialised state

    /**
      @brief   Serialises this descriptor to a JSON object.
    **/
    [[nodiscard]] JsonValue              ToJson() const;

    /**
      @brief   Deserialises a BehaviorDesc from a JSON object.
      @param   j  JsonValue of object type containing "type" and "properties".
    **/
    [[nodiscard]] static BehaviorDesc    FromJson(const JsonValue &j);
  };

  /**
    @struct  ObjectDesc
    @brief   Plain-data snapshot of one game object.
    @details Captures the object's name, UUID, transform, and the list of
             behavior descriptors needed to recreate it via SceneIO::Apply().
  **/
  struct ObjectDesc {
    String             name;        //!< Display name of the object
    UUID               uuid{};      //!< Persistent identifier; preserved across save/load
    Transform          transform{}; //!< World-space position, rotation, and scale
    List<BehaviorDesc> behaviors;   //!< Ordered list of behavior snapshots

    /**
      @brief   Serialises this descriptor to a JSON object.
    **/
    [[nodiscard]] JsonValue             ToJson() const;

    /**
      @brief   Deserialises an ObjectDesc from a JSON object.
      @param   j  JsonValue of object type containing "name", "uuid", "transform", and "behaviors".
    **/
    [[nodiscard]] static ObjectDesc     FromJson(const JsonValue &j);
  };

  /**
    @struct  SceneDesc
    @brief   Plain-data snapshot of an entire scene.
    @details Contains the scene name and the list of object descriptors needed
             to restore the scene from disk via SceneIO::Apply().
  **/
  struct SceneDesc {
    String            name;    //!< Scene name
    List<ObjectDesc>  objects; //!< All objects in the scene

    /**
      @brief   Serialises this descriptor to a JSON object.
    **/
    [[nodiscard]] JsonValue            ToJson() const;

    /**
      @brief   Deserialises a SceneDesc from a JSON object.
      @param   j  JsonValue of object type containing "name" and "objects".
    **/
    [[nodiscard]] static SceneDesc     FromJson(const JsonValue &j);

    /**
      @brief   Loads a SceneDesc from a .scene JSON file on disk.
      @param   filePath  Path to the .scene file.
    **/
    [[nodiscard]] static SceneDesc     LoadFromFile(const char *filePath);

    /**
      @brief   Writes this descriptor to a .scene JSON file on disk.
      @param   filePath  Destination path; parent directory must exist.
    **/
    void                               SaveToFile(const char *filePath) const;
  };

} // namespace ROSE
