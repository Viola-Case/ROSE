/**

    @file      ROSE_mathenum.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      25.02.2026
    @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/ROSE_preamble.h>

namespace ROSE::math {
	/**
			@struct Sign
			@brief  it's a sign lol
	**/
	struct Sign {
		enum Value : int8_t { Negative = -1, Zero = 0, Positive = 1 } value;
		constexpr Sign(Value v) : value(v) {}
		constexpr Sign(float i) : value((i == 0 ? Value::Zero : (i > 0 ? Value::Positive : Value::Negative))) {}
		constexpr operator int8_t() const noexcept { return static_cast<int8_t>(value); }
	};

	constexpr Sign SignOf(float num) noexcept {
		return	(num > 0.f ? Sign::Positive :
			(num < 0.f ? Sign::Negative : Sign::Zero));
	}

	constexpr bool IsPositive(const Sign s, bool incZero = true) noexcept {
		return (s == Sign::Positive) || (incZero && s == Sign::Zero);
	}

	template<typename T>
	constexpr bool KDelta(T a, T b) { return (a == b); }

	/**
		@brief  Returns the sign of the permutation of arguments (aka "Levi-Civita symbol")
		@tparam Args - template parameter pack type
		@param  args - template parameter pack, use `size_t`
		@retval      -
	**/

	template <typename... Args>
	constexpr Sign LeviCivita(Args... args) {
		static_assert((std::is_convertible_v<Args, size_t> && ...), "cse::math::LeviCivita only accepts size_t-convertible arguments");
		constexpr size_t N = sizeof...(Args);
		size_t arr[N] = { args... };

		size_t maxval = 0;
		for (size_t i = 0; i < N; ++i) {
			if (arr[i] > maxval) maxval = arr[i];
		}
		if (maxval != N - 1) return Sign::Zero;

		int sign = 1;
		for (size_t i = 0; i < N; ++i) {
			for (size_t j = i + 1; j < N; ++j) {
				if (arr[i] > arr[j]) sign = -sign;
			}
		}

		return (sign == 1 ? Sign::Positive : Sign::Negative);
	}

}