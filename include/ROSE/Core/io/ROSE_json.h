/**

  @file      ROSE_json.h
  @brief     JSON type alias and ROSE math/UUID type converters
  @details   ~
  @author    Viola Case
  @date      19.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <nlohmann/json.hpp>
#include <ROSE/Core/ROSE_uuid.h>
#include <ROSE/Core/ROSE_transform.h>

namespace ROSE {
  using Json = nlohmann::json;

  [[nodiscard]] Json   JsonFromUUID(const UUID &uuid) noexcept;
  [[nodiscard]] UUID   JsonToUUID(const Json &j);

  [[nodiscard]] Json   JsonFromVec3d(const Vec3d &v) noexcept;
  [[nodiscard]] Vec3d  JsonToVec3d(const Json &j);

  [[nodiscard]] Json   JsonFromQuatd(const Quatd &q) noexcept;
  [[nodiscard]] Quatd  JsonToQuatd(const Json &j);

  [[nodiscard]] Json      JsonFromTransform(const Transform &t) noexcept;
  [[nodiscard]] Transform JsonToTransform(const Json &j);
}
