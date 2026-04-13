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
  class Scene {
    friend class Application;
  public:
    [[nodiscard]] Application &GetApplication() const noexcept;
  private:
    TypedHashMap<UUID, UniquePtr<Object>> m_objects{};

    void FrameUpdate() noexcept;
    Application *m_application{nullptr};

  };
}

