
#include <ROSE/Core/rtl/ROSE_bigint.h>


#if !(defined(INT_128_EXISTS))
namespace ROSE {

#pragma region int128_t
  int128_t::int128_t() noexcept = default;

  int128_t::int128_t(const int128_t &) noexcept = default;

  int128_t::int128_t(int64_t i) noexcept : high(0), low(i) {}
#pragma endregion

#pragma region uint128_t
  uint128_t::uint128_t() noexcept = default;
  uint128_t::uint128_t(const uint128_t &) noexcept = default;
  uint128_t::uint128_t(uint64_t i) noexcept : high(0), low(i) {}

#pragma endregion

}

#endif
