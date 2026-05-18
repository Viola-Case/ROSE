/**

  @file      ROSE_camera.h
  @brief
  @details   ~
  @author    Viola Case
  @date      08.04.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/

#pragma once
#include <ROSE/Core/ROSE_behavior.h>
#include <ROSE/Core/ROSE_math.h>


namespace ROSE {
  class Camera : public Behavior {
    math::Vec2<int16_t> m_AspectRatio;
    [[bounds({ 0, inf })]] float m_FocalLength { 30 }; //!< millimeters
    bool m_Orthographic { false };

  public:
  };
} // namespace ROSE
