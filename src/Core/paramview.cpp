/**

  @file       paramview.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       23.06.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/Core/ROSE_paramview.h>
#include <nlohmann/json.hpp>

namespace ROSE {
  int ParamView::GetInt(const String &key, int fallback) {
    if (!m_node) return fallback;
    nlohmann::json j = *(nlohmann::json*)m_node;
  }
  double ParamView::GetDouble(const String &key, double fallback) const noexcept {
    if (!m_node) return fallback;
  }
  bool ParamView::GetBool(const String &key, bool fallback) const noexcept {
    if (!m_node) return fallback;
  }
  String ParamView::GetString(const String &key, const String &fallback) const {
    if (!m_node) return fallback;
  }
  UUID ParamView::GetUUID(const String &key) const noexcept {
    if (!m_node) return {0};
  }
  ParamView ParamView::Child(const String &key) const noexcept {
    if (!m_node) return fallback;
  }
} // namespace ROSE