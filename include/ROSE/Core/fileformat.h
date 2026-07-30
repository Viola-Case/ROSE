/**

  @file       fileformat.h
  @brief
  @details    ~
  @author     Viola Case
  @date       05.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <cstdint>

#include <ROSE/Core/utility.h>

namespace ROSE {

  enum class FileType : uint32_t {
    Unknown = 0,
    Asset = CharTagToInt32("ASET"),
    Archive = CharTagToInt32("ARCH"),
    Scene = CharTagToInt32("SCNE"),
    Plugin = CharTagToInt32("PLGN"),

  };

  struct FileHeader {
    uint8_t magic[4] { 'R', 'O', 'S', 'E' };
    FileType type;
    constexpr FileHeader(FileType _type) noexcept : type(_type) {}
  };
} // namespace ROSE
