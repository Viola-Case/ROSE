/**

  @file       motion.h
  @brief
  @details    ~
  @author     Viola Case
  @date       13.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once
#include <ROSE/Core/api.h>
#include <ROSE/Core/math/vector.h>
#include <ROSE/Core/behavior.h>
namespace ROSE {
  class ROSE_API(Core) Motion : public Behavior {
  public:
    static constexpr UUID typeID = "ab0a57d02d8e9fde-462c4cdfe26597d3"_uuid;
    static constexpr UUID TypeID() noexcept { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    void SetVelocity(const Vec3d &);
    void SetAcceleration(const Vec3d &);
    void SetAngularVelocity(const Vec3d &);
    void SetAngularAcceleration(const Vec3d &);

    Vec3d &GetVelocity() noexcept;
    Vec3d &GetAcceleration() noexcept;
    Vec3d &GetAngularVelocity() noexcept;
    Vec3d &GetAngularAcceleration() noexcept;

  protected:
    Vec3d m_drdt{}; //!< \f$\dv{\overset{\rightharpoonup}{r}}{t}\f$
    Vec3d m_dTdt{}; //!< \f$\dv{\overset{\rightharpoonup}{\theta}}{t}\f$
    Vec3d m_d2rdt2{}; //!< \f$\dv[2]{\overset{\rightharpoonup}{r}}{t}\f$
    Vec3d m_d2Tdt2{}; //!< \f$\dv[2]{\overset{\rightharpoonup}{\theta}}{t}\f$

    /*!
     * Unpack takes JSON object
     * {
     *    "drdt": [
     *      0,0,0
     *    ],
     *    "dTdt": [
     *      0,0,0
     *    ]
     * }
     */
    void Unpack(const ParamView &view) override;
    void FrameUpdate() override;
  };
} // namespace ROSE