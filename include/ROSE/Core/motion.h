/**

  @file       motion.h
  @brief
  @details    ~
  @author     Viola Case
  @date       13.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once
#include <ROSE/Core/math/vector.h>
#include <ROSE/Core/behavior.h>
namespace ROSE {
  class Motion : public Behavior {
  public:
    static constexpr UUID typeID = "ab0a57d02d8e9fde-462c4cdfe26597d3"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }
  protected:
    Vec3d m_drdt{}; //!< \f$\dv{r}{t}\f$
    Vec3d m_dTdt{}; //!< \f$\dv{\theta}{t}\f$
    /*!
     * Unpack takes JSON object {
     *    "velocity",
     *    "
     * }
     */
    void Unpack(const ParamView &view) override;
    void FrameUpdate() override;
  };
} // namespace ROSE