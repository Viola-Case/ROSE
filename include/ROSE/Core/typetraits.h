/**

    @file      typetraits.h
    @brief
    @details   ~
    @author    Viola Case
    @date      17.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/stdlib.h>

namespace ROSE {
  template <typename T>
  concept Character =
      std::same_as<T, char> ||
      std::same_as<T, signed char> ||
      std::same_as<T, unsigned char> ||
      std::same_as<T, wchar_t> ||
      std::same_as<T, char8_t> ||
      std::same_as<T, char16_t>
      //|| std::same_as<T, char32_t>
      ;

  template <typename T>
  concept StdScalar = std::is_arithmetic_v<T>;

  namespace math {
    template <StdScalar T>
    struct Comp;
    template <StdScalar T>
    struct Quat;
  } // namespace math

  template <typename T>
  concept Scalar =
      std::same_as<T, math::Comp<std::underlying_type_t<T>>> ||
      std::same_as<T, math::Quat<std::underlying_type_t<T>>> ||
      StdScalar<T>;

  template <typename T>
  concept MultiByteType =
      sizeof(T) > 1;

  class Behavior;
  template <class T>
  concept BehaviorType = std::is_base_of_v<Behavior, T>;
} // namespace ROSE