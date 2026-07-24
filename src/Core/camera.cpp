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
    /* TODO GetDouble only accepts JSON floats, so a whole-number "focalLength": 30 is
     * dropped for the fallback. See the TODO on ParamView::GetDouble. */
    m_focalLength = view.GetDouble("focalLength",30);
    // do something with the aspect ratio
    /* TODO m_orthographic is a bool - read it with GetBool. Through GetDouble a JSON
     * true/false never matches is_number_float(), so this is always the fallback. */
    m_orthographic = view.GetDouble("orthographic",0);

  }
} // namespace ROSE
