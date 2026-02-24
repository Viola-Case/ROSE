/**

    @file      ROSE_behavior.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

namespace ROSE {
  class Behavior {
    virtual void OnStart() = 0;
    virtual void FrameUpdate() = 0;
    virtual void FixedUpdate() = 0;
  };

  using Behaviour = Behavior;
}