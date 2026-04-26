#pragma once

#include <cstdint>

#include <ROSE/Core/rtl/ROSE_utility.h>

#if (defined (__clang__) || defined (__GNUC__))
#define INT_128_EXISTS
#endif

#if (defined (INT_128_EXISTS))
using int128_t = __int128_t;
using uint128_t = __uint128_t;
#else

namespace ROSE {
  struct int128_t {
    int64_t high; uint64_t low;

    int128_t() noexcept;
    int128_t(const int128_t &) noexcept;
    int128_t(int64_t) noexcept;
  };

  struct uint128_t {
    uint64_t high, low;

    uint128_t() noexcept;

    uint128_t(const uint128_t &) noexcept;

    uint128_t(uint64_t) noexcept;

    uint128_t &operator= (const uint128_t &) noexcept;
    uint128_t &operator+=(const uint128_t &) noexcept;
    uint128_t &operator-=(const uint128_t &) noexcept;
    uint128_t &operator*=(const uint128_t &) noexcept;
    uint128_t &operator/=(const uint128_t &) noexcept;
    uint128_t &operator%=(const uint128_t &) noexcept;
    uint128_t &operator&=(const uint128_t &) noexcept;
    uint128_t &operator|=(const uint128_t &) noexcept;
    uint128_t &operator^=(const uint128_t &) noexcept;
    uint128_t &operator<<=(const uint128_t &) noexcept;
    uint128_t &operator>>=(const uint128_t &) noexcept;
    uint128_t operator+(const uint128_t &) const noexcept;
    uint128_t operator-(const uint128_t &) const noexcept;
    uint128_t operator*(const uint128_t &) const noexcept;
    uint128_t operator/(const uint128_t &) const noexcept;
    uint128_t operator%(const uint128_t &) const noexcept;
    uint128_t operator&(const uint128_t &) const noexcept;
    uint128_t operator|(const uint128_t &) const noexcept;
    uint128_t operator^(const uint128_t &) const noexcept;
    uint128_t operator<<(const uint128_t &) const noexcept;
    uint128_t operator>>(const uint128_t &) const noexcept;
    auto operator<=>(const uint128_t &) const noexcept;
  };
}

using ROSE::int128_t;
using ROSE::uint128_t;

#endif

namespace ROSE {
  constexpr  int128_t operator""_lll (const char* str) {
    int128_t result{};
    bool neg = false;
    if (str[0] == '-') neg = true;
    if (!neg) {
      if (StrLen(str) > 2 && str[0] == '0') {
        if ( ToLower(str[1]) == 'x') {
          // parse hex
          for (int i = 2; str[i] != '\0'; ++i) {
            const auto c = ToLower(str[i]);
            if (c >= '0' && c <= '9') {
              result *= 16ull;
              result += c - '0';
            }
            else if (c >= 'a' && c <= 'f') {
              result *= 16ull;
              result += (c - 'a' + 10);
            } else if (c == '\'') {
              continue;
            } else throw std::invalid_argument("invalid digit");
          }
        } else if (ToLower(str[1]) == 'b') {
          // parse binary
          for (int i = 2; str[i] != '\0'; ++i) {
            const auto c = str[i];
            if (c == '0' || c == '1') {
              result <<= 1;
              result += (c-'0');
            } else if (c == '\'') {
              continue;
            } else throw std::invalid_argument("invalid digit");
          }
        } else {
          // parse octal
          for (int i = 1; str[i] != '\0'; ++i) {
            const auto c = str[i];
            if (c >= '0' && c <= '7') {
              result <<= 3;
              result += c - '0';
            } else if (c == '\'') {
              continue;
            } else throw std::invalid_argument("invalid digit");
          }
        }
      } else {
        // parse dec
        for (int i = 0; i < StrLen(str); ++i) {
          auto c = str[i];
          result *= 10;
          if (c >= '0' && c <= '9')
            result += c - '0';
          else if (c == '\'') {
            continue;
          }
          else throw std::invalid_argument("invalid digit");
        }
      }
    } else {
      if (StrLen(str) > 3 && str[1] == '0') {
        if ( ToLower(str[2]) == 'x') {
          // parse hex
          for (int i = 3; str[i] != '\0'; ++i) {
            const auto c = ToLower(str[i]);
            if (c >= '0' && c <= '9') {
              result *= 16ull;
              result += c - '0';
            }
            else if (c >= 'a' && c <= 'f') {
              result *= 16ull;
              result += (c - 'a' + 10);
            } else if (c == '\'') {
              continue;
            } else throw std::invalid_argument("invalid digit");
          }
        } else if (ToLower(str[2]) == 'b') {
          // parse binary
          for (int i = 3; str[i] != '\0'; ++i) {
            const auto c = str[i];
            if (c == '0' || c == '1') {
              result <<= 1;
              result += (c-'0');
            } else if (c == '\'') {
              continue;
            } else throw std::invalid_argument("invalid digit");
          }
        } else {
          // parse octal
          for (int i = 2; str[i] != '\0'; ++i) {
            const auto c = str[i];
            if (c >= '0' && c <= '7') {
              result <<= 3;
              result += c - '0';
            } else if (c == '\'') {
              continue;
            } else throw std::invalid_argument("invalid digit");
          }
        }
      } else {
        // parse dec
        for (int i = 1; str[i] != '\0'; ++i) {
          auto c = str[i];
          result *= 10;
          if (c >= '0' && c <= '9')
            result += c - '0';
          else if (c == '\'') {
            continue;
          }
          else throw std::invalid_argument("invalid digit");
        }
      }
    }
    return result;
  } //!<@todo change to parse helper
  constexpr uint128_t operator""_ulll(const char* str) {
    uint128_t result{};
    if (StrLen(str) > 2 && str[0] == '0') {
      if ( ToLower(str[1]) == 'x') {
        // parse hex
        for (int i = 2; str[i] != '\0'; ++i) {
          const auto c = ToLower(str[i]);
          if (c >= '0' && c <= '9') {
            result *= 16ull;
            result += c - '0';
          }
          else if (c >= 'a' && c <= 'f') {
            result *= 16ull;
            result += (c - 'a' + 10);
          } else if (c == '\'') {
            continue;
          } else throw std::invalid_argument("invalid digit");
        }
      } else if (ToLower(str[1]) == 'b') {
        // parse binary
        for (int i = 2; str[i] != '\0'; ++i) {
          const auto c = str[i];
          if (c == '0' || c == '1') {
            result <<= 1;
            result += (c-'0');
          } else if (c == '\'') {
            continue;
          } else throw std::invalid_argument("invalid digit");
        }
      } else {
        // parse octal
        for (int i = 1; str[i] != '\0'; ++i) {
          const auto c = str[i];
          if (c >= '0' && c <= '7') {
            result <<= 3;
            result += c - '0';
          } else if (c == '\'') {
            continue;
          } else throw std::invalid_argument("invalid digit");
        }
      }
    } else {
      // parse dec
      for (int i = 0; i < StrLen(str); ++i) {
        auto c = str[i];
        result *= 10;
        if (c >= '0' && c <= '9')
          result += c - '0';
        else if (c == '\'') {
          continue;
        }
        else throw std::invalid_argument("invalid digit");
      }
    }
    return result;
  } //!<@todo change to parse helper
  constexpr  int128_t operator""_128 (const char* str) {
    return operator""_lll(str);
  }
  constexpr uint128_t operator""_u128(const char* str) {
    return operator""_ulll(str);
  }
}
