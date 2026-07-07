/**

  @file      ROSE_scene.h
  @brief
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_rtl.h>
#include <ROSE/Core/ROSE_uuid.h>

namespace ROSE {
  class Application;
  class Object;

  class Scene final {
    friend class Application;
    friend class Object;

  public:
    Application &GetApplication() const noexcept;

    void AddObject(Object &&) noexcept;
    void DestroyObject(const UUID &u) noexcept;

    static Scene FromJSONString(const String&) noexcept;
  private:
    Scene();

    Scene(Scene &&) noexcept = default;
    Scene &operator=(Scene &&) noexcept = default;


    void OnStart() noexcept;
    void FrameUpdate() noexcept;

    Object *GetObject(const UUID &) noexcept;

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
