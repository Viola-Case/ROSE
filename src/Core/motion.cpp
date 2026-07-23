/**

  @file       motion.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       13.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>

namespace ROSE {
  void Motion::Unpack(const ParamView &view) {
    m_drdt = view.GetVec3d("drdt", {});
    m_dTdt = view.GetVec3d("dTdt", {});
  }
  void Motion::FrameUpdate() {
    const double &dt = Time::deltaTime;
    Transform &tran = GetObject().transform;

    /* Semi-implicit Euler: the rates advance first so the integration below runs against end-of-frame velocities. Costs
     * nothing over the explicit form and does not pump energy into the system when acceleration is nonzero.*/
    m_drdt += (m_d2rdt2 * dt);
    m_dTdt += (m_d2Tdt2 * dt);

    tran.position += (m_drdt * dt);

    /* dTdt is an angular velocity: its direction is the axis and its magnitude
     * the rate in rad/s, so over dt it sweeps |dTdt|·dt radians about that axis.
     * Composing that delta on the left applies it in world space, matching the
     * way position is integrated in world space above. */
    const double omega = math::Sqrt(m_dTdt.dot(m_dTdt));
    if (omega > 0.0) {
      const double inv = 1.0 / omega;
      const Quatd delta = Quatd::AxisAngle(omega * dt, m_dTdt.x * inv, m_dTdt.y * inv, m_dTdt.z * inv);
      tran.rotation = delta * tran.rotation;
      /* Products accumulate rounding error, which shows up as the object slowly
       * scaling once the quaternion is turned into a matrix. */
      tran.rotation.Normalize();
    }
  }
} // namespace ROSE