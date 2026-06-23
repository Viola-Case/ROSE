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
    static inline Atomic<uint64_t> s_counter { 0 };

    uint128_t value;
    struct {
      uint64_t high, low;
    };

    [[nodiscard]] constexpr bool operator==(const UUID &_other) const noexcept {
      return value == _other.value;
    }

    [[nodiscard]] static UUID Generate() noexcept;

    [[nodiscard]] static UUID Generate(const char *str) noexcept;
  };

} // namespace ROSE

template <>
struct std::hash<ROSE::UUID> {
  size_t operator()(const ROSE::UUID &uuid) const noexcept {
    return ROSE::FNV1A64(&uuid, sizeof(ROSE::UUID));
  }
};

// TODO make UUID literal parser and formatter
// std::format should take only one parser spec, which would return the raw integer instead of the nicer-looking
// `high000000000000-low0000000000000` format. Usually not very useful unless working in assembly.
