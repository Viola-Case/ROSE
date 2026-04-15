/**

  @file      ROSE_scene.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once
#include <ROSE/Core/ROSE_object.h>

//class Application;

namespace ROSE {
  class Scene final {
    friend class Application;
    friend class Object;
  public:
    Application &GetApplication() const noexcept;
  private:
    void AddObject(Object &&) noexcept;
    void DestroyObject(const UUID &u) noexcept;

    void OnStart() noexcept;
    void FrameUpdate() noexcept;

    Object *GetObject(const UUID &) noexcept;


  private:
    TypedHashMap<UUID, UniquePtr<Object>> m_objects{};
    List<UniquePtr<Object>> m_pendingAdd{};
    List<const UUID> m_pendingDestroy{};
    Application *m_application{nullptr};
  };
}

