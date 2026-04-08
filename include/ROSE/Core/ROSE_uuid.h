/**

    @file      ROSE_uuid.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_stdlib.h>

namespace ROSE {
  union UUID {
    struct {
      uint64_t high;
      uint64_t low;
    };
    uint64_t data[2];

  };
}