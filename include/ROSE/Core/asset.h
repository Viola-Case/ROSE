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
    Texture = Tag("TXTR"),
    Audio = Tag("AUDO"),
    Mesh = Tag("MESH"),
    Shader = Tag("SHDR"),
    Prefab = Tag("PRFB"),
    Material = Tag("MTRL"),
    Animation = Tag("ANIM"),
    Font = Tag("FONT"),
    Script = Tag("SRPT"),
    Raw = Tag("RAWF"),



  };

  using AssetFlags = uint32_t;
  constexpr AssetFlags AssetFlags_None = 0;
  constexpr AssetFlags AssetFlags_Compressed = 1 << 6;

}