/**

  @file       ROSE_paramview.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       22.06.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once

#include <ROSE/Core/ROSE_string.h>
#include <ROSE/Core/ROSE_uuid.h>

namespace ROSE {
  /*!
   * @note missing key and wrong-typed key both fall through to the fallback today.
   * These are different animals — missing = honest version skew (limp forward),
   * wrong-type = possible corruption (deserves a louder channel). Split them when
   * the corruption-detection story is real. Until then: survive everything, detect nothing.
   *
   * @var m_node - pointer to a json node
   */
  class ParamView {
  public:
    ParamView(const void*) noexcept;
    int      GetInt   (const String& key, int      fallback) const noexcept;
    double   GetDouble(const String& key, double   fallback) const noexcept;
    bool     GetBool  (const String& key, bool     fallback) const noexcept;
    String   GetString(const String& key, const String& fallback) const;
    UUID     GetUUID  (const String& key) const noexcept;
    ParamView Child   (const String& key) const noexcept;   // nested objects

  private:
    const void* m_node;
  };
}