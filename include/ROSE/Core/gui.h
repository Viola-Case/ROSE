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
  /*!
   * @todo There is no gui.cpp - none of the three overrides below are defined, so any
   * target that instantiates a UI fails to link. It is also the only Core behavior
   * missing from the ROSE.h umbrella header, and it isn't registered in
   * RoseRegisterCoreModule, so it can't be reached from a scene file either.
   */
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