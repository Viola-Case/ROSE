/**

  @file       bigint.h
  @brief
  @details    ~
  @author     Viola Case
  @date       10.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.

 **/

#pragma once

#include <cstdint>

#include <ROSE/Core/utility.h>

#if (defined(__clang__) || defined(__GNUC__))
  #define INT_128_EXISTS
#endif

#if (defined(INT_128_EXISTS))
using int128_t = __int128_t;
using uint128_t = __uint128_t;
#else

#error "THIS IS NOT A FUNCTIONAL IMPLEMENTATION!"

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
      const uint64_t new_high = hh + (lh >> 32) + (hl >> 32) + (mid >> 32) + low * rhs.high + high * rhs.low;
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
      if (rhs.high || rhs.low >= 128) {
        high = low = 0;
      } else {
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
      if (rhs.high || rhs.low >= 128) {
        high = low = 0;
      } else {
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
      auto result { *this };
      return (result += rhs);
    }

    constexpr uint128_t operator-(const uint128_t &rhs) const noexcept {
      auto result { *this };
      return (result -= rhs);
    }

    constexpr uint128_t operator*(const uint128_t &rhs) const noexcept {
      auto result { *this };
      return (result *= rhs);
    }

    constexpr uint128_t operator/(const uint128_t &rhs) const noexcept {
      auto result { *this };
      return (result /= rhs);
    }

    constexpr uint128_t operator%(const uint128_t &rhs) const noexcept {
      auto result { *this };
      return (result %= rhs);
    }

    constexpr uint128_t operator&(const uint128_t &rhs) const noexcept {
      auto result { *this };
      return (result &= rhs);
    }

    constexpr uint128_t operator|(const uint128_t &rhs) const noexcept {
      auto result { *this };
      return (result |= rhs);
    }

    constexpr uint128_t operator^(const uint128_t &rhs) const noexcept {
      auto result { *this };
      return (result ^= rhs);
    }

    constexpr uint128_t operator<<(const uint128_t &rhs) const noexcept {
      auto result { *this };
      return (result <<= rhs);
    }

    constexpr uint128_t operator>>(const uint128_t &rhs) const noexcept {
      auto result { *this };
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
} // namespace ROSE

using ROSE::int128_t;
using ROSE::uint128_t;

#endif

namespace ROSE {

  /**
   * @note DO NOT CALL THIS AT RUNTIME. We try not to throw any sort of `std::exception` in the engine logic so this
   * should only ever be statically evaluated.
   */
  constexpr uint128_t parse128(const char *str, size_t idx) {
    uint128_t result {};
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
        if (c >= '0' && c <= '9') result += c - '0';
        else if (c == '\'') {
          continue;
        } else throw std::invalid_argument("invalid digit");
      }
    }
    return result;
  }

  // "cannot convert unsigned long long to const char*" errors on these literals are a known Clang false positive —
  // ignore them.
  constexpr int128_t operator""_lll(const char *str) {
    if (str[0] == '-') return -parse128(str, 1);
    else return parse128(str, 0);
  }

  // "cannot convert unsigned long long to const char*" errors on these literals are a known Clang false positive —
  // ignore them.
  constexpr uint128_t operator""_ulll(const char *str) { return static_cast<uint128_t>(parse128(str, 0)); }
  // "cannot convert unsigned long long to const char*" errors on these literals are a known Clang false positive —
  // ignore them.
  constexpr int128_t operator""_128(const char *str) { return operator""_lll(str); }
  // "cannot convert unsigned long long to const char*" errors on these literals are a known Clang false positive —
  // ignore them.
  constexpr uint128_t operator""_u128(const char *str) { return operator""_ulll(str); }
} // namespace ROSE

template <>
struct std::formatter<uint128_t> {
  char presentation = 'd';
  bool alt_form = false;
  bool zero_pad = false;
  int width = 0;

  constexpr auto parse(std::format_parse_context &ctx) {
    auto it = ctx.begin();
    while (it != ctx.end() && *it != '}') {
      switch (*it) {
      case '#':
        alt_form = true;
        ++it;
        break;
      case '0':
        zero_pad = true;
        ++it;
        break;
      case 'd':
      case 'x':
      case 'X':
      case 'b':
      case 'B':
      case 'o':
        presentation = *it++;
        return it;
      default:
        if (*it >= '1' && *it <= '9') {
          while (it != ctx.end() && *it >= '0' && *it <= '9')
            width = width * 10 + (*it++ - '0');
        } else {
          throw std::format_error("invalid format spec for uint128_t");
        }
      }
    }
    return it;
  }

  auto format(uint128_t val, std::format_context &ctx) const {
    char buf[130];
    char *end = buf + sizeof(buf);
    char *ptr = end;
    *--ptr = '\0';

    if (val == 0) {
      *--ptr = '0';
    } else {
      switch (presentation) {
      case 'd': {
        while (val > 0) {
          *--ptr = '0' + (val % 10);
          val /= 10;
        }
        break;
      }
      case 'x': {
        while (val > 0) {
          *--ptr = "0123456789abcdef"[val & 0xF];
          val >>= 4;
        }
        break;
      }
      case 'X': {
        while (val > 0) {
          *--ptr = "0123456789ABCDEF"[val & 0xF];
          val >>= 4;
        }
        break;
      }
      case 'b':
      case 'B': {
        while (val > 0) {
          *--ptr = '0' + (val & 1);
          val >>= 1;
        }
        break;
      }
      case 'o': {
        while (val > 0) {
          *--ptr = '0' + (val & 7);
          val >>= 3;
        }
        break;
      }
      }
    }

    // prefix
    char prefix[4] = {};
    if (alt_form) {
      switch (presentation) {
      case 'x':
        prefix[0] = '0';
        prefix[1] = 'x';
        break;
      case 'X':
        prefix[0] = '0';
        prefix[1] = 'X';
        break;
      case 'b':
      case 'B':
        prefix[0] = '0';
        prefix[1] = 'b';
        break;
      case 'o':
        prefix[0] = '0';
        break;
      }
    }

    std::string_view digits(ptr, end - ptr - 1);
    std::string_view pre(prefix, std::char_traits<char>::length(prefix));

    int total = (int)(pre.size() + digits.size());
    int pad = std::max(0, width - total);

    auto out = ctx.out();
    if (zero_pad) {
      out = std::copy(pre.begin(), pre.end(), out);
      for (int i = 0; i < pad; ++i)
        *out++ = '0';
    } else {
      for (int i = 0; i < pad; ++i)
        *out++ = ' ';
      out = std::copy(pre.begin(), pre.end(), out);
    }
    return std::copy(digits.begin(), digits.end(), out);
  }
};

template <>
struct std::formatter<int128_t> {
  char presentation = 'd';
  bool alt_form = false;
  bool zero_pad = false;
  int width = 0;
  char sign_mode = '-'; // '-' = only negative, '+' = always, ' ' = space for positive

  constexpr auto parse(std::format_parse_context &ctx) {
    auto it = ctx.begin();
    while (it != ctx.end() && *it != '}') {
      switch (*it) {
      case '#':
        alt_form = true;
        ++it;
        break;
      case '0':
        zero_pad = true;
        ++it;
        break;
      case '+':
        sign_mode = '+';
        ++it;
        break;
      case ' ':
        sign_mode = ' ';
        ++it;
        break;
      case 'd':
      case 'x':
      case 'X':
      case 'b':
      case 'B':
      case 'o':
        presentation = *it++;
        return it;
      default:
        if (*it >= '1' && *it <= '9') {
          while (it != ctx.end() && *it >= '0' && *it <= '9')
            width = width * 10 + (*it++ - '0');
        } else {
          throw std::format_error("invalid format spec for int128_t");
        }
      }
    }
    return it;
  }
  template <typename FormatContext>
  auto format(int128_t val, FormatContext &ctx) const {
    char buf[130];
    char *end = buf + sizeof(buf);
    char *ptr = end;
    *--ptr = '\0';

    bool negative = val < 0;
    int128_t uval = negative ? -static_cast<int128_t>(val) : static_cast<int128_t>(val);

    if (uval == 0) {
      *--ptr = '0';
    } else {
      switch (presentation) {
      case 'd': {
        while (uval > 0) {
          *--ptr = '0' + (uval % 10);
          uval /= 10;
        }
        break;
      }
      case 'x': {
        while (uval > 0) {
          *--ptr = "0123456789abcdef"[uval & 0xF];
          uval >>= 4;
        }
        break;
      }
      case 'X': {
        while (uval > 0) {
          *--ptr = "0123456789ABCDEF"[uval & 0xF];
          uval >>= 4;
        }
        break;
      }
      case 'b':
      case 'B': {
        while (uval > 0) {
          *--ptr = '0' + (uval & 1);
          uval >>= 1;
        }
        break;
      }
      case 'o': {
        while (uval > 0) {
          *--ptr = '0' + (uval & 7);
          uval >>= 3;
        }
        break;
      }
      }
    }

    char prefix[6] = {};
    char *pfx = prefix;
    if (negative) *pfx++ = '-';
    else if (sign_mode == '+') *pfx++ = '+';
    else if (sign_mode == ' ') *pfx++ = ' ';

    if (alt_form) {
      switch (presentation) {
      case 'x':
        *pfx++ = '0';
        *pfx++ = 'x';
        break;
      case 'X':
        *pfx++ = '0';
        *pfx++ = 'X';
        break;
      case 'b':
      case 'B':
        *pfx++ = '0';
        *pfx++ = 'b';
        break;
      case 'o':
        *pfx++ = '0';
        break;
      }
    }

    std::string_view digits(ptr, end - ptr - 1);
    std::string_view pre(prefix, pfx - prefix);

    int total = (int)(pre.size() + digits.size());
    int pad = std::max(0, width - total);

    auto out = ctx.out();
    if (zero_pad) {
      out = std::copy(pre.begin(), pre.end(), out);
      for (int i = 0; i < pad; ++i)
        *out++ = '0';
    } else {
      for (int i = 0; i < pad; ++i)
        *out++ = ' ';
      out = std::copy(pre.begin(), pre.end(), out);
    }
    return std::copy(digits.begin(), digits.end(), out);
  }
};
