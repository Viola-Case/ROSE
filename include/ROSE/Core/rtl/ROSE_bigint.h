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
  struct int128_t;
  struct uint128_t;

  struct int128_t {
    int64_t high;
    uint64_t low;

    int128_t() noexcept;

    int128_t(const int128_t &) noexcept;

    int128_t(int64_t) noexcept;

    int128_t(uint128_t) noexcept;

    constexpr int128_t &operator=(const int128_t &) noexcept;

    constexpr int128_t &operator+=(const int128_t &) noexcept;

    constexpr int128_t &operator-=(const int128_t &) noexcept;

    constexpr int128_t &operator*=(const int128_t &) noexcept;

    constexpr int128_t &operator/=(const int128_t &) noexcept;

    constexpr int128_t &operator%=(const int128_t &) noexcept;

    constexpr int128_t &operator&=(const int128_t &) noexcept;

    constexpr int128_t &operator|=(const int128_t &) noexcept;

    constexpr int128_t &operator^=(const int128_t &) noexcept;

    constexpr int128_t &operator<<=(const int128_t &) noexcept;

    constexpr int128_t &operator>>=(const int128_t &) noexcept;

    constexpr int128_t operator+(const int128_t &) const noexcept;

    constexpr int128_t operator-(const int128_t &) const noexcept;

    constexpr int128_t operator*(const int128_t &) const noexcept;

    constexpr int128_t operator/(const int128_t &) const noexcept;

    constexpr int128_t operator%(const int128_t &) const noexcept;

    constexpr int128_t operator&(const int128_t &) const noexcept;

    constexpr int128_t operator|(const int128_t &) const noexcept;

    constexpr int128_t operator^(const int128_t &) const noexcept;

    constexpr int128_t operator<<(const int128_t &) const noexcept;

    constexpr int128_t operator>>(const int128_t &) const noexcept;

    constexpr auto operator<=>(const int128_t &) const noexcept;
  };

  struct uint128_t {
    uint64_t high, low;

    uint128_t() noexcept;

    uint128_t(const uint128_t &) noexcept;

    constexpr uint128_t(uint64_t) noexcept;

    constexpr uint128_t(int128_t) noexcept;

    constexpr uint128_t &operator=(const uint128_t &rhs) noexcept {
      high = rhs.high;
      low = rhs.low;
      return *this;
    }

    constexpr uint128_t &operator+=(const uint128_t &rhs) noexcept {
      const uint64_t prev = low;
      low += rhs.low;
      high += rhs.high + (low < prev ? 1u : 0u);
      return *this;
    }

    constexpr uint128_t &operator-=(const uint128_t &rhs) noexcept {
      const uint64_t prev = low;
      low -= rhs.low;
      high -= rhs.high + (low > prev ? 1u : 0u);
      return *this;
    }

    constexpr uint128_t &operator*=(const uint128_t &rhs) noexcept {
      const uint64_t a_lo = low & 0xFFFFFFFFull, a_hi = low >> 32;
      const uint64_t b_lo = rhs.low & 0xFFFFFFFFull, b_hi = rhs.low >> 32;
      const uint64_t ll = a_lo * b_lo, lh = a_lo * b_hi;
      const uint64_t hl = a_hi * b_lo, hh = a_hi * b_hi;
      const uint64_t mid = (ll >> 32) + (lh & 0xFFFFFFFFull) + (hl & 0xFFFFFFFFull);
      const uint64_t new_high = hh + (lh >> 32) + (hl >> 32) + (mid >> 32)
                                + low * rhs.high + high * rhs.low;
      low = (mid << 32) | (ll & 0xFFFFFFFFull);
      high = new_high;
      return *this;
    }

  private:
    constexpr void divmod(const uint128_t &n, const uint128_t &d, uint128_t &q, uint128_t &r) {
      q = uint128_t(0);
      r = uint128_t(0);
      if (d.high == 0 && d.low == 0) return;
      for (int i = 127; i >= 0; --i) {
        r <<= uint128_t(1);
        r.low |= (i >= 64 ? (n.high >> (i - 64)) : (n.low >> i)) & 1ull;
        if (r.high > d.high || (r.high == d.high && r.low >= d.low)) {
          r -= d;
          if (i >= 64) q.high |= 1ull << (i - 64);
          else q.low |= 1ull << i;
        }
      }
    }

  public:
    /*constexpr uint128_t &operator/=(const uint128_t &rhs) noexcept {
      uint128_t q, r;
      divmod(*this, rhs, q, r);
      return *this = q;
    }*/



    constexpr uint128_t &operator&=(const uint128_t &rhs) noexcept {
      high &= rhs.high;
      low &= rhs.low;
      return *this;
    }

    constexpr uint128_t &operator|=(const uint128_t &rhs) noexcept {
      high |= rhs.high;
      low |= rhs.low;
      return *this;
    }

    constexpr uint128_t &operator^=(const uint128_t &rhs) noexcept {
      high ^= rhs.high;
      low ^= rhs.low;
      return *this;
    }

    constexpr uint128_t &operator<<=(const uint128_t &rhs) noexcept {
      if (rhs.high || rhs.low >= 128) { high = low = 0; } else {
        const uint64_t n = rhs.low;
        if (n == 0) {
        } else if (n >= 64) {
          high = low << (n - 64);
          low = 0;
        } else {
          high = (high << n) | (low >> (64 - n));
          low <<= n;
        }
      }
      return *this;
    }

    constexpr uint128_t &operator>>=(const uint128_t &rhs) noexcept {
      if (rhs.high || rhs.low >= 128) { high = low = 0; } else {
        const uint64_t n = rhs.low;
        if (n == 0) {
        } else if (n >= 64) {
          low = high >> (n - 64);
          high = 0;
        } else {
          low = (low >> n) | (high << (64 - n));
          high >>= n;
        }
      }
      return *this;
    }

    constexpr uint128_t operator+(const uint128_t &rhs) const noexcept {
      auto result{*this};
      return (result += rhs);
    }

    constexpr uint128_t operator-(const uint128_t &rhs) const noexcept {
      auto result{*this};
      return (result -= rhs);
    }

    constexpr uint128_t operator*(const uint128_t &rhs) const noexcept {
      auto result{*this};
      return (result *= rhs);
    }

    constexpr uint128_t operator/(const uint128_t &rhs) const noexcept {
      auto result{*this};
      return (result /= rhs);
    }

    constexpr uint128_t operator%(const uint128_t &rhs) const noexcept {
      auto result{*this};
      return (result %= rhs);
    }

    constexpr uint128_t operator&(const uint128_t &rhs) const noexcept {
      auto result{*this};
      return (result &= rhs);
    }

    constexpr uint128_t operator|(const uint128_t &rhs) const noexcept {
      auto result{*this};
      return (result |= rhs);
    }

    constexpr uint128_t operator^(const uint128_t &rhs) const noexcept {
      auto result{*this};
      return (result ^= rhs);
    }

    constexpr uint128_t operator<<(const uint128_t &rhs) const noexcept {
      auto result{*this};
      return (result <<= rhs);
    }

    constexpr uint128_t operator>>(const uint128_t &rhs) const noexcept {
      auto result{*this};
      return (result >>= rhs);
    }

    constexpr auto operator<=>(const uint128_t &rhs) const noexcept {
      if (auto c = high <=> rhs.high; c != 0) return c;
      return low <=> rhs.low;
    }

    constexpr uint128_t &operator%=(const uint128_t &rhs) noexcept {
      while (*this >= rhs) {
        *this -= rhs;
      }
      return *this;
    }
  };
}

using ROSE::int128_t;
using ROSE::uint128_t;

#endif

namespace ROSE {
  constexpr uint128_t parse128(const char *str, size_t idx) {
    uint128_t result{};
    if (StrLen(str) > 2 + idx && str[idx] == '0') {
      if (ToLower(str[1 + idx]) == 'x') {
        // parse hex
        for (int i = 2 + idx; str[i] != '\0'; ++i) {
          const auto c = ToLower(str[i]);
          if (c >= '0' && c <= '9') {
            result *= 16ull;
            result += c - '0';
          } else if (c >= 'a' && c <= 'f') {
            result *= 16ull;
            result += (c - 'a' + 10);
          } else if (c == '\'') {
            continue;
          } else throw std::invalid_argument("invalid digit");
        }
      } else if (ToLower(str[1 + idx]) == 'b') {
        // parse binary
        for (int i = 2 + idx; str[i] != '\0'; ++i) {
          const auto c = str[i];
          if (c == '0' || c == '1') {
            result <<= 1;
            result += (c - '0');
          } else if (c == '\'') {
            continue;
          } else throw std::invalid_argument("invalid digit");
        }
      } else {
        // parse octal
        for (int i = 1 + idx; str[i] != '\0'; ++i) {
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
      for (int i = 0 + idx; i < StrLen(str); ++i) {
        auto c = str[i];
        result *= 10;
        if (c >= '0' && c <= '9')
          result += c - '0';
        else if (c == '\'') {
          continue;
        } else throw std::invalid_argument("invalid digit");
      }
    }
    return result;
  }

  // "cannot convert unsigned long long to const char*" errors on these literals are a known Clang false positive — ignore them.
  constexpr int128_t operator""_lll(const char *str) {
    if (str[0] == '-')
      return -parse128(str, 1);
    else
      return parse128(str, 0);
  }

  // "cannot convert unsigned long long to const char*" errors on these literals are a known Clang false positive — ignore them.
  constexpr uint128_t operator""_ulll(const char *str) {
    return static_cast<uint128_t>(parse128(str, 0));
  }
// "cannot convert unsigned long long to const char*" errors on these literals are a known Clang false positive — ignore them.
  constexpr int128_t operator""_128(const char *str) {
    return operator""_lll(str);
  }
// "cannot convert unsigned long long to const char*" errors on these literals are a known Clang false positive — ignore them.
  constexpr uint128_t operator""_u128(const char *str) {
    return operator""_ulll(str);
  }
}
