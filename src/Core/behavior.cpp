/**

    @file      behavior.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      08.04.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/ROSE.h>

namespace ROSE {
  Object &Behavior::GetObject() noexcept { return *m_object; }
}