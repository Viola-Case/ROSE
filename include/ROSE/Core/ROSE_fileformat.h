/**

  @file       ROSE_fileformat.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       05.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once

#include <cstdint>

#include <ROSE/Core/ROSE_utility.h>

namespace ROSE {

  enum class FileType : uint32_t {
    Unknown = 0,
    Asset = Tag("ASET"),
    Archive = Tag("ARCH"),
    Scene = Tag("SCNE"),
    Plugin = Tag("PLGN"),

  };

  struct FileHeader {
    uint8_t magic[4]{'R','O','S','E'};
    FileType type;
    constexpr FileHeader(FileType _type) noexcept : type(_type) {}
  };
}
