/**

  @file       paramview.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       23.06.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/Core/ROSE_bigint.h>


#include <ROSE/Core/ROSE_utility.h>
#include <ROSE/Core/ROSE_paramview.h>
#include <nlohmann/json.hpp>

namespace ROSE {
  ParamView::ParamView() noexcept : m_node(nullptr) {}
  ParamView::ParamView(const void *node) noexcept : m_node(node) {}

  int ParamView::GetInt(const String &key, int fallback) const noexcept {
    if (!m_node) return fallback;
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_number_integer()) return fallback;
    const auto &v = it->object();
    if (!v.is_number()) return fallback;
    return v.get<int>();
  }
  double ParamView::GetDouble(const String &key, double fallback) const noexcept {
    if (!m_node) return fallback;
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_number_float()) return fallback;
    const auto &v = it->object();
    if (!v.is_number_float()) return fallback;
    return v.get<double>();
  }
  bool ParamView::GetBool(const String &key, bool fallback) const noexcept {
    if (!m_node) return fallback;
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_boolean()) return fallback;
    const auto &v = it->object();
    if (!v.is_boolean()) return fallback;
    return v.get<bool>();
  }
  String ParamView::GetString(const String &key, const String &fallback) const {
    if (!m_node) return fallback;
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_string()) return fallback;
    const auto &v = it->object();
    if (!v.is_string()) return fallback;
    return v.get<String>();
  }
  UUID ParamView::GetUUID(const String &key) const noexcept {
    // #if defined(UUID_THROW_ON_FAILURE)
    // #define fallback() throw
    // #else
    #define fallback() return UUID::Invalid()
    // #endif
    if (!m_node) fallback();
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_string()) fallback();
    const auto &v = *it;
    auto s = v.get<String>();
    if (s.size() != ROSE_UUID_HIGH_LEN + ROSE_UUID_SEPARATOR + ROSE_UUID_LOW_LEN) fallback();
    // todo check for corrupted bytes in uuid
    const uint128_t high = strtoull(s.c_str(), nullptr, 16);
    const uint64_t low  = strtoull(s.c_str() + ROSE_UUID_HIGH_LEN + ROSE_UUID_SEPARATOR, nullptr, 16);
    return { (high << 64) | low };
    #undef fallback
  }
  ParamView ParamView::Child(const String &key) const noexcept {
    ParamView fallback {};
    if (!m_node) return fallback;
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_object()) return fallback;
    const auto &v = *it;
    ParamView child;
    child.m_node = &v;
    return child;
  }
} // namespace ROSE