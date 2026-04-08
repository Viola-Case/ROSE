/**

  @file      ROSE_scene.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once
#include <ROSE/Core/ROSE_application.h>
#include <ROSE/Core/ROSE_object.h>

namespace ROSE {
  class Scene {
    TypedHashMap<UUID, UniquePtr<Object>> m_objects{};

    Application *m_application{nullptr};
  public:
    Application &GetApplication() const noexcept;
  };
}

