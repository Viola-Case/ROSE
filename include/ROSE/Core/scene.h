/**

  @file      scene.h
  @brief
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/rtl.h>
#include <ROSE/Core/uuid.h>

namespace ROSE {
  class Application;
  class Object;

  class Scene final {
    friend class Application;
    friend class Object;

  public:
    Application &GetApplication() const noexcept;

    void AddObject(Object &&) noexcept;

    Object *FindObjectByName(const StringView &) noexcept;

    Object *GetObject(const UUID &) noexcept;

    void DestroyObject(const UUID &u) noexcept;

    static Scene FromJSONString(const String&) noexcept;
    Scene(Scene &&) noexcept = default;
    Scene &operator=(Scene &&) noexcept = default;

  private:
    Scene();



    void OnStart() noexcept;
    void FrameUpdate() noexcept;

    /* Promotes every object's pending behaviors into its behavior map and runs
     * their Create -> Start lifecycle. Loops until the scene settles so that
     * behaviors spawned during OnCreate/OnStart are themselves initialized
     * before the next update. */
    void InitializePendingBehaviors() noexcept;

    // Points this scene's application/object back-pointers at their final
    // addresses. Must be re-called if the scene is ever moved.
    void Bind(Application &) noexcept;


    String ToJSONString() noexcept;

  private:
    String m_name{"Scene"};
    TypedHashMap<UUID, UniquePtr<Object>> m_objects {};
    List<UniquePtr<Object>> m_pendingAdd {};
    List<UUID> m_pendingDestroy {};
    Application *m_application { nullptr };
  };

  class SceneManager final {
    friend class Application;
    
  };

} // namespace ROSE
