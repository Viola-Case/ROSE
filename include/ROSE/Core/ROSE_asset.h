/**

    @file      ROSE_asset.h
    @brief
    @details   ~
    @author    Viola Case
    @date      05.05.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

namespace ROSE {

  enum class AssetType : uint32_t {
    Texture         = Tag("TXTR"),
    Audio           = Tag("AUDO"),
    Mesh            = Tag("MESH"),
    Shader          = Tag("SHDR"),
    Prefab          = Tag("PRFB"),
    Material        = Tag("MTRL"),
    Animation       = Tag("ANIM"),
    Font            = Tag("FONT"),
    Script          = Tag("SRPT"),


  };

}