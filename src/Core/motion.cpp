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

  }
} // namespace ROSE