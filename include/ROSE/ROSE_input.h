/**

    @file      ROSE_input.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      3.03.2026
    @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/ROSE_preamble.h>

namespace ROSE {
  struct KeyState {
    bool Pressed : 1;
    bool Down : 7;
  };
  /**
      @enum  ROSE::KeyCode
      @brief Think I'll sync it with SDL's scancodes or smth

  **/
  enum class KeyCode {
    
  };
}