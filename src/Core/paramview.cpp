/**

  @file       paramview.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       23.06.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/Core/math.h>


#include <ROSE/Core/bigint.h>


#include <ROSE/Core/utility.h>
#include <ROSE/Core/paramview.h>
#include <nlohmann/json.hpp>

/**
 * Set this to 1 if you want GetUUID to throw on failure. Don't touch this if you don't know what you're doing.
 */
#define UUID_THROW_ON_FAILURE 0

namespace ROSE {
  ParamView::ParamView() noexcept : m_node(nullptr) {}
  ParamView::ParamView(const void *node) noexcept : m_node(node) {}

  int ParamView::GetInt(const String &key, int fallback) const noexcept {
    if (!m_node) return fallback;
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    const auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_number()) return fallback;
    const auto &v = *it;
    return v.get<int>();
  }

  double ParamView::GetDouble(const String &key, double fallback) const noexcept {
    if (!m_node) return fallback;
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    const auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_number_float()) return fallback;
    const auto &v = *it;
    return v.get<double>();
  }

  bool ParamView::GetBool(const String &key, bool fallback) const noexcept {
    if (!m_node) return fallback;
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    const auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_boolean()) return fallback;
    const auto &v = *it;
    return v.get<bool>();
  }

  String ParamView::GetString(const String &key, const String &fallback) const noexcept {
    if (!m_node) return fallback;
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    const auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_string()) return fallback;
    const auto &v = *it;
    return String(v.get_ref<const std::string&>().c_str());
  }

  UUID ParamView::GetUUID(const String &key) const noexcept {
    #if UUID_THROW_ON_FAILURE
    #define FALLBACK() throw
    #else
    #define FALLBACK() return UUID::Invalid()
    #endif
    if (!m_node) FALLBACK();
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    const auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_string()) FALLBACK();
    const auto &v = *it;
    auto s = String(v.get_ref<const std::string&>().c_str());
    if (s.size() != ROSE_UUID_HIGH_LEN + ROSE_UUID_SEPARATOR_LEN + ROSE_UUID_LOW_LEN) FALLBACK();
    // todo check for corrupted bytes in uuid
    const uint128_t high = strtoull(s.c_str(), nullptr, 16);
    const uint64_t low  = strtoull(s.c_str() + ROSE_UUID_HIGH_LEN + ROSE_UUID_SEPARATOR_LEN, nullptr, 16);
    return { (high << 64) | low };
    #undef FALLBACK
  }

  ParamView ParamView::Child(const String &key) const noexcept {
    const ParamView fallback {};
    if (!m_node) return fallback;
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    const auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_object()) return fallback;
    const auto &v = *it;
    ParamView child;
    child.m_node = &v;
    return child;
  }

  Vec3d ParamView::GetVec3d(const String &key, const Vec3d fallback) const noexcept {
    if (!m_node) return fallback;
    const auto *node = static_cast<const nlohmann::json*>(m_node);
    const auto it = node->find(key.c_str());
    if (it == node->end() || !it->is_array() || it->size() != 3) return fallback;
    Vec3d vec;
    const auto &arr = *it;
    if (!arr[0].is_number() || !arr[1].is_number() || !arr[2].is_number()) return fallback;
    vec.x = arr[0].get<double>();
    vec.y = arr[1].get<double>();
    vec.z = arr[2].get<double>();
    return vec;
  }



  const void *ParamView::GetNode() const noexcept { return m_node; }
} // namespace ROSE