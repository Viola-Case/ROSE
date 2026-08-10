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
  // dT and deltaTime are deliberately not `inline`: inline statics have vague
  // linkage, so a module linking Core would fold its own copy rather than share
  // the one Application::Run() writes. They are defined out of line in
  // application.cpp instead.
  class Time {
    friend class Application;
    static double dT;

  public:
    static const double &deltaTime;
  };
} // namespace ROSE