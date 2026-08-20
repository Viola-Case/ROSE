/**

    @file      uuid.h
    @brief
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/api.h>
#include <ROSE/Core/stdlib.h>
#include <ROSE/Core/utility.h>
#include <ROSE/Core/bigint.h>

namespace ROSE {
  struct UUID {
    constexpr UUID() = default;
    constexpr UUID(const UUID &) = default;
    constexpr UUID(uint128_t val) noexcept : value(val) {}
    constexpr UUID(uint64_t h, uint64_t l) noexcept : high(h), low(l) {}

    union {
      uint128_t value;
      struct {
        uint64_t high, low;
      };
    };

    [[nodiscard]] constexpr bool operator==(const UUID &_other) const noexcept {
      return value == _other.value;
    }

    [[nodiscard]] ROSE_API(CORE) static UUID Generate() noexcept;

    constexpr static UUID Invalid() noexcept { return {}; }
  };

  constexpr size_t ROSE_UUID_HIGH_LEN = 16;
  constexpr size_t ROSE_UUID_SEPARATOR_LEN = 1;
  constexpr size_t ROSE_UUID_LOW_LEN = 16;


  constexpr size_t ROSE_UUID_STR_LEN = ROSE_UUID_HIGH_LEN + ROSE_UUID_SEPARATOR_LEN + ROSE_UUID_LOW_LEN;

  class bad_uuid : public std::exception {};

  constexpr UUID operator ""_uuid(const char *str, size_t len) {
    if (len < ROSE_UUID_STR_LEN) throw bad_uuid();

    const uint128_t high = HexToULL(str);
    const uint64_t low  = HexToULL(str + ROSE_UUID_HIGH_LEN + ROSE_UUID_SEPARATOR_LEN);
    return { (high << 64) | low };
  }

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




