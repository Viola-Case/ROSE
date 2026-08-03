/**

  @file       time.h
  @brief
  @details    ~
  @author     Viola Case
  @date       15.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

namespace ROSE {
  class Time {
    friend class Application;
    static inline double dT {};

  public:
    const inline static double &deltaTime { dT };
  };

  #define Δt Time.deltaTime
} // namespace ROSE