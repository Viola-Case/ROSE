/**

  @file       AMinput.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       11.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "assetmaker_input.h"
#include <array>
#include <ranges>
#include <string_view>
#include <algorithm>
using namespace ROSE;
AssetType ParseAssetExtensionType(const char *extension) {
  std::string ext { extension };
  for (auto &c : ext) {
    c = ToLower(c);
  }
  {
    constexpr auto textureExts = std::array { "png", "jpg", "jpeg", "bmp", "tga", "tiff", "dds" };
    if (std::ranges::any_of(textureExts, [&](auto s) { return ext == s; })) return AssetType::Texture;
  }
  {
    constexpr auto meshExts = std::array { "obj", "fbx", "gltf", "glb" };
    if (std::ranges::any_of(meshExts, [&](auto s) { return ext == s; })) return AssetType::Mesh;
  }
  {
    constexpr auto audioExts = std::array { "ogg", "mp3", "flac", "wav" };
    if (std::ranges::any_of(audioExts, [&](auto s) { return ext == s; })) return AssetType::Audio;
  }
  {
    constexpr auto shaderExts = std::array { "glsl", "hlsl", "vert", "frag", "shader", "cg", "fxh", "hlsli" };
    if (std::ranges::any_of(shaderExts, [&](auto s) { return ext == s; })) return AssetType::Shader;
  }
  {
    constexpr auto fontExts = std::array { "ttf", "otf", "svg" };
    if (std::ranges::any_of(fontExts, [&](auto s) { return ext == s; })) return AssetType::Font;
  }
  {
    constexpr auto scriptExts = std::array { "lua", "luac", "rosescript" };
    if (std::ranges::any_of(scriptExts, [&](auto s) { return ext == s; })) return AssetType::Script;
  }
  if (ext == "anim" || ext == "roseanim") return AssetType::Animation;


  return AssetType::Raw;
}