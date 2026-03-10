/**

    @file      ROSE_utility.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      17.02.2026
    @copyright © Cool Guy, 2026. All right reserved.

**/
#pragma once

#include <utility>
#include <ROSE/preamble/ROSE_typetraits.h>

namespace ROSE {

  void MemCpy(void *_Dst, const void *_Src, size_t size);

  template<typename T>
  constexpr std::remove_reference_t<T> &&Move(T &t) noexcept { return static_cast<std::remove_reference_t<T> &&>(t); }

  template<typename T>
  constexpr T &&Forward(std::remove_reference_t<T> &t) noexcept { return static_cast<T &&>(t); }

  template<typename T>
  constexpr T &&Forward(std::remove_reference_t<T> &&t) noexcept {
    static_assert(!std::is_lvalue_reference_v<T>,
      "bad forward: cannot forward rvalue as lvalue");
    return static_cast<T &&>(t);
  }

  template<typename T>
  constexpr void Swap(T &A, T &B) {
    T temp{ Move(A)};
    A = Move(B);
    B = Move(temp);
  }

  template<typename T, typename U = T>
  constexpr T Exchange(T &obj, U &&newval) {
    T old = Move(obj);
    obj = Forward<U>(newval);
    return old;
  }

#pragma region Byte swapping

  constexpr uint16_t ByteSwap(uint16_t v) noexcept {
    return (v >> 8) | (v << 8);
  }

  constexpr uint32_t ByteSwap(uint32_t v) noexcept {
    return
      (v >> 24) |
      ((v >> 8) & 0x0000ff00u) |
      ((v << 8) & 0x00ff0000u) |
      (v << 24);
  }

  constexpr uint64_t ByteSwap(uint64_t v) noexcept {
    return
      (v >> 56) |
      ((v >> 40) & 0x000000000000ff00u) |
      ((v >> 24) & 0x0000000000ff0000u) |
      ((v >> 8) & 0x00000000ff000000u) |
      ((v << 8) & 0x000000ff00000000u) |
      ((v << 24) & 0x0000ff0000000000u) |
      ((v << 40) & 0x00ff000000000000u) |
      (v << 56);
  }

  template<typename T>
  concept ByteSwapResult =
    std::is_same_v<T, uint16_t> ||
    std::is_same_v<T, uint32_t> ||
    std::is_same_v<T, uint64_t>;

  template<ByteSwapResult T, typename U>
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

  enum class HashFunction {
    MurmurHash,
    FNV_1a,
    CityHash,
    XXHash
  };

  template<typename Key>
  constexpr uint64_t Hash(HashFunction func, const Key &key) {
    switch (func) {
    case HashFunction::MurmurHash:
      return;
    case HashFunction::FNV_1a:
      return;
    case HashFunction::CityHash:
      return;
    case HashFunction::XXHash:
      return;
    }
  }
}