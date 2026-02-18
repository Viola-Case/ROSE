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
    virtual void OnStart();
    virtual void FrameUpdate();
    virtual void FixedUpdate();
  };

  using Behaviour = Behavior;
}