/**

    @file      api.cpp
    @brief     
    @details   ~
    @author    Viola Case
    @date      27.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/Core/api.h>

extern "C" const char *ROSE_GetBaseline() {
  #ifdef ROSE_BASELINE
  return ROSE_BASELINE;
  #else
  return "";
  #endif
}
