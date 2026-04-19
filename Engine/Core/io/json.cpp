/**

  @file      json.cpp
  @brief     JSON converter implementations for ROSE math and UUID types
  @details   ~
  @author    Viola Case
  @date      19.04.2026
  @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/Core/io/ROSE_json.h>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace ROSE {
  Json JsonFromUUID(const UUID &uuid) noexcept {
    char buf[34];
    snprintf(buf, sizeof(buf), "%016llx-%016llx",
             static_cast<unsigned long long>(uuid.high),
             static_cast<unsigned long long>(uuid.low));
    return std::string(buf);
  }

  UUID JsonToUUID(const Json &j) {
    const std::string s = j.get<std::string>();
    UUID uuid{};
    if (s.size() == 33) {
      uuid.high = static_cast<uint64_t>(strtoull(s.substr(0, 16).c_str(), nullptr, 16));
      uuid.low  = static_cast<uint64_t>(strtoull(s.substr(17, 16).c_str(), nullptr, 16));
    }
    return uuid;
  }

  Json JsonFromVec3d(const Vec3d &v) noexcept {
    return {{"x", v.x}, {"y", v.y}, {"z", v.z}};
  }

  Vec3d JsonToVec3d(const Json &j) {
    return {
      j.at("x").get<double>(),
      j.at("y").get<double>(),
      j.at("z").get<double>()
    };
  }

  Json JsonFromQuatd(const Quatd &q) noexcept {
    return {{"w", q.w}, {"x", q.x}, {"y", q.y}, {"z", q.z}};
  }

  Quatd JsonToQuatd(const Json &j) {
    return {
      j.at("w").get<double>(),
      j.at("x").get<double>(),
      j.at("y").get<double>(),
      j.at("z").get<double>()
    };
  }

  Json JsonFromTransform(const Transform &t) noexcept {
    return {
      {"position", JsonFromVec3d(t.position)},
      {"rotation", JsonFromQuatd(t.rotation)},
      {"scale",    JsonFromVec3d(t.scale)}
    };
  }

  Transform JsonToTransform(const Json &j) {
    return {
      JsonToVec3d(j.at("position")),
      JsonToQuatd(j.at("rotation")),
      JsonToVec3d(j.at("scale"))
    };
  }
}
