/**

  @file       fpscounter.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       19.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#define ROSE_LOGLEVELS_USE_NERDFONT_SYMBOLS
#include "fpscounter.h"
#include <imgui.h>

constexpr double refreshInterval { .125 };

using namespace ROSE;

void FpsCounter::OnStart() {

}

void FpsCounter::FrameUpdate() {
  m_fAccum++;
  m_tAccum += Time::deltaTime;
  if (m_tAccum >= refreshInterval) {
    m_fps = m_fAccum / m_tAccum;
    m_fAccum = 0;
    m_tAccum = 0;
  }
  if (ImGui::Begin("FPS")) {
    ImGui::Text("%s", Format("FPS: {:.2f}", m_fps).c_str());
    ImGui::End();
  }
}
