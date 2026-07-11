/**

  @file      metadata.h
  @brief
  @details
  @author    Viola Case
  @date      7.04.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <cstdint>

namespace ROSE::Editor {
  struct GameMetaData {
    const char *c_compiler;
    const uint64_t c_compiler_version;
    const char *cpp_compiler;
    const uint64_t cpp_compiler_version;
    const char *linker;
    const uint64_t linker_version;
  };
} // namespace ROSE::Editor