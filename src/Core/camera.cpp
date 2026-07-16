/**

  @file       camera.cpp
  @brief      
  @details    ~
  @author     Viola Case
  @date       15.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/

#include <ROSE/Core/camera.h>

#include <ROSE/Core/paramview.h>

namespace ROSE {
  void Camera::Unpack(const ParamView &view) {
    m_focalLength = view.GetDouble("focalLength",30);
    // do something with the aspect ratio
    m_orthographic = view.GetDouble("orthographic",0);
    
  }
} // namespace ROSE
