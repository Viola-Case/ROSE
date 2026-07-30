/**

    @file      asset.h
    @brief
    @details   ~
    @author    Viola Case
    @date      05.05.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <cstdint>

#include <ROSE/Core/utility.h>

namespace ROSE {

  enum class AssetType : uint32_t {
    Texture = CharTagToInt32("TXTR"),
    Audio = CharTagToInt32("AUDO"),
    Mesh = CharTagToInt32("MESH"),
    Shader = CharTagToInt32("SHDR"),
    Prefab = CharTagToInt32("PRFB"),
    Material = CharTagToInt32("MTRL"),
    Animation = CharTagToInt32("ANIM"),
    Font = CharTagToInt32("FONT"),
    Script = CharTagToInt32("SRPT"),
    Raw = CharTagToInt32("RAWF"),



  };

  using AssetFlags = uint32_t;
  constexpr AssetFlags AssetFlags_None = 0;
  constexpr AssetFlags AssetFlags_Compressed = 1 << 6;

}