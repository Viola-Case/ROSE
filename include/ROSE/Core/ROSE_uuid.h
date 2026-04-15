/**

    @file      ROSE_uuid.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_stdlib.h>

namespace ROSE {
  union UUID {
    struct {
      uint64_t high;
      uint64_t low;
    };
    uint64_t data[2];

    [[nodiscard]] constexpr bool operator==(const UUID &_other) const noexcept {
      return data[0] == _other.data[0] && data[1] == _other.data[1];
    }
  };
}

template<>
struct std::hash<ROSE::UUID> {
  size_t operator()(const ROSE::UUID& uuid) const noexcept {
    size_t h1 = std::hash<uint64_t>{}(uuid.high);
    size_t h2 = std::hash<uint64_t>{}(uuid.low);
    return h1 ^ (h2 * 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
  }
};