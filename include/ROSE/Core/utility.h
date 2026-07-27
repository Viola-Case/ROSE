/**

    @file      utility.h
    @brief
    @details   ~
    @author    Viola Case
    @date      17.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <utility>
#include <ROSE/Core/typetraits.h>

namespace ROSE {
  /// might implement thread safety a different way later but for now im gonna keep everything nice and easy and
  /// comfortable and not awful by just using `std::atomic` instead of custom multithreading terribleness
  template <typename T>
  using Atomic = std::atomic<T>;

  /*!
   * Only allows implicit conversion
   * @return
   */
  template <typename U>
  constexpr U ImplicitCast(std::type_identity_t<U> &&v) noexcept {
    return v;
  }

  template <typename T>
  constexpr std::remove_reference_t<T> &&Move(T &t) noexcept { return static_cast<std::remove_reference_t<T> &&>(t); }

  template <typename T>
  constexpr T &&Forward(std::remove_reference_t<T> &t) noexcept { return static_cast<T &&>(t); }

  template <typename T>
  constexpr T &&Forward(std::remove_reference_t<T> &&t) noexcept {
    static_assert(!std::is_lvalue_reference_v<T>,
                  "bad forward: cannot forward rvalue as lvalue");
    return static_cast<T &&>(t);
  }

  template <typename T>
  constexpr void Swap(T &A, T &B) {
    T temp { Move(A) };
    A = Move(B);
    B = Move(temp);
  }

  template <typename T>
  constexpr T Min(T a, T b) { return a < b ? a : b; }

  template <typename T>
  constexpr T Max(T a, T b) { return a > b ? a : b; }

  template <typename T, typename U = T>
  constexpr T Exchange(T &obj, U &&newval) {
    T old = Move(obj);
    obj = Forward<U>(newval);
    return old;
  }

  void MemCpy(void *_Dst, const void *_Src, size_t size);

  template <typename T, typename U>
  constexpr void SmartMemCpy(T *_Dst, U *_Src, size_t count = 1) {
    MemCpy(_Dst, _Src, Min(sizeof(T), sizeof(U)));
  } // TODO either remove the constexpr or inline `MemCpy`

  /**
      @brief  constexpr element-wise memcmp replacement; safe to call with null pointers
      @tparam T     - element type; count is in elements, not bytes
      @param  a     - left buffer (null compares less than non-null, equal to null)
      @param  b     - right buffer
      @param  count - number of elements to compare
      @retval       - <0, 0 or >0 like memcmp
  **/
  template <typename T>
  constexpr int MemCmp(const T *a, const T *b, size_t count) noexcept {
    if (count == 0 || a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    for (size_t i = 0; i < count; ++i) {
      if (a[i] != b[i]) {
        // memcmp orders bytes as unsigned, so integral types compare unsigned here too
        if constexpr (std::is_integral_v<T>) {
          using U = std::make_unsigned_t<T>;
          return U(a[i]) < U(b[i]) ? -1 : 1;
        } else {
          return a[i] < b[i] ? -1 : 1;
        }
      }
    }
    return 0;
  }

#pragma region Byte swapping

  constexpr uint16_t ByteSwap(uint16_t v) noexcept {
    return (v >> 8) | (v << 8);
  }

  constexpr uint32_t ByteSwap(uint32_t v) noexcept {
    return (v >> 24) |
           ((v >> 8) & 0x0000ff00u) |
           ((v << 8) & 0x00ff0000u) |
           (v << 24);
  }

  constexpr uint64_t ByteSwap(uint64_t v) noexcept {
    return (v >> 56) |
           ((v >> 40) & 0x000000000000ff00u) |
           ((v >> 24) & 0x0000000000ff0000u) |
           ((v >> 8) & 0x00000000ff000000u) |
           ((v << 8) & 0x000000ff00000000u) |
           ((v << 24) & 0x0000ff0000000000u) |
           ((v << 40) & 0x00ff000000000000u) |
           (v << 56);
  }

  template <typename T>
  concept ByteSwapResult =
      std::is_same_v<T, uint16_t> ||
      std::is_same_v<T, uint32_t> ||
      std::is_same_v<T, uint64_t>;

  template <ByteSwapResult T, typename U>
    requires std::is_fundamental_v<U>
  constexpr T ByteSwap(U v) noexcept {
    return ByteSwap(static_cast<T>(v));
  }
#pragma endregion

  constexpr const size_t NextPow2(size_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
  }

  /**
      @brief  General StrLen template for any Character type
      @tparam CharT - character type
      @param  str   - input string
      @retval       - length of string
  **/
  template <Character CharT>
  constexpr size_t StrLen(const CharT *str) noexcept {
    if (!str) return 0;
    size_t len = 0;
    while (str[len] != CharT(0))
      ++len;
    return len;
  }

  constexpr uint32_t Tag(const char (&s)[5]) noexcept {
    if (StrLen(s) < 4) return 0;
    return (uint32_t(s[3]) << 24) | (uint32_t(s[2]) << 16) | (uint32_t(s[1]) << 8) | (uint32_t(s[0]));
  }

  constexpr int ToLower(const int c) {
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
  }

  /**
      @brief  constexpr hexadecimal string -> uint64_t parser (replacement for strtoull with base 16)
      @param  str - input string; skips leading whitespace and an optional "0x"/"0X" prefix, then
                    consumes hex digits until the first non-hex character
      @retval     - parsed value, or 0 if str is null / has no leading hex digits
  **/
  constexpr uint64_t StrToULL(const char *str) noexcept {
    if (!str) return 0;

    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r' || *str == '\f' || *str == '\v')
      ++str;

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
      str += 2;

    uint64_t value = 0;
    for (; *str != '\0'; ++str) {
      const int c = *str;
      uint64_t digit;
      if (c >= '0' && c <= '9')      digit = uint64_t(c - '0');
      else if (c >= 'a' && c <= 'f') digit = uint64_t(c - 'a' + 10);
      else if (c >= 'A' && c <= 'F') digit = uint64_t(c - 'A' + 10);
      else break;

      value = (value << 4) | digit;
    }
    return value;
  }

  uint64_t FNV1A64(const void *data, size_t len);
  uint64_t FNV1A64(const char *str);


} // namespace ROSE
