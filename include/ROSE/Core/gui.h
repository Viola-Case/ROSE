/**

  @file       gui.h
  @brief
  @details    ~
  @author     Viola Case
  @date       19.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/behavior.h>
namespace ROSE {
  class UI : public Behavior {
  public:
    static constexpr UUID typeID = "5be3e81fa226cdc4-c8105a5b51659aa0"_uuid;
    static constexpr UUID TypeID() { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

  protected:
    void FrameUpdate() override;
    void OnStart() override;
    void OnEnable() override;
  };
} // namespace ROSE