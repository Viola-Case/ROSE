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
    static constexpr UUID typeID = UUID{0x98b16c050e6597982ba97b3cd1a9dd7c_u128};
    static constexpr UUID TypeID() { return typeID; }
  };
} // namespace ROSE
