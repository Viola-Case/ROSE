/**

    @file      ROSE_uuid.h
    @brief     UUID type — 128-bit universally unique identifier
    @details   UUID is a plain-data union of two uint64_t values. Use
               UUID::Generate() to create a cryptographically random identifier.
               Two UUIDs are equal if and only if both halves match.
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_stdlib.h>

namespace ROSE {
  /**
    @union   UUID
    @brief   128-bit universally unique identifier.
    @details Stored as two 64-bit halves (`high` / `low`) or as a two-element
             array (`data`). All fields share the same storage.
  **/
  union UUID {
    struct {
      uint64_t high; //!< Upper 64 bits
      uint64_t low;  //!< Lower 64 bits
    };
    uint64_t data[2]; //!< Raw 64-bit halves as an array

    /**
      @brief   Compares two UUIDs for equality (both halves must match).
    **/
    [[nodiscard]] constexpr bool operator==(const UUID &_other) const noexcept {
      return data[0] == _other.data[0] && data[1] == _other.data[1];
    }

    /**
      @brief   Generates a new random UUID.
      @retval  A UUID whose bits are sourced from a cryptographic RNG.
    **/
    [[nodiscard]] static UUID Generate() noexcept;
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
