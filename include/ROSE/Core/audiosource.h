/**

  @file       audiosource.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       10.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once
#include <ROSE/Core/behavior.h>
#include <ROSE/Core/audio.h>


namespace ROSE {
  class AudioSource : public Behavior {
  public:
    static constexpr UUID typeID = "8448e5b94997ad4d-ccee5b44f06598a2"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

  private:


  };
}