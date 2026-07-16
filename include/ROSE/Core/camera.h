/**

  @file      camera.h
  @brief
  @details   ~
  @author    Viola Case
  @date      08.04.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/

#pragma once
#include <ROSE/Core/behavior.h>
#include <ROSE/Core/math.h>


namespace ROSE {
  class Camera : public Behavior {
  protected:
    math::Vec2<int16_t> m_aspectRatio; //!< I should probably make this just a single float
    [[bounds({ 0, inf })]] float m_focalLength { 30 }; //!< millimeters
    bool m_orthographic { false };
    void Unpack(const ParamView &view) override;

  public:
    static constexpr UUID typeID = "98b16c050e659798-2ba97b3cd1a9dd7c"_uuid;
    static constexpr UUID TypeID() { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }
  };
} // namespace ROSE
