/**

  @file       time.h
  @brief
  @details    ~
  @author     Viola Case
  @date       15.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/api.h>

namespace ROSE {
  /* dT and deltaTime are deliberately not `inline`: inline statics have vague linkage, so an executable linking
   * ROSE_Core.dll would fold its own copy and read 0.0 forever while the DLL updated its own. They are defined out of
   * line in application.cpp and reach consumers through the import table instead. */
  class ROSE_API(Core) Time {
    friend class Application;
    static double dT;

  public:
    static const double &deltaTime;
  };
} // namespace ROSE