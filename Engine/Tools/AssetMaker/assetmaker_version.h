/**

  @file       assetmaker_version.h
  @brief
  @details    ~
  @author     Viola Case
  @date       10.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_version.h>

#define ASSET_MAKER_VERSION_MAJOR 0
#define ASSET_MAKER_VERSION_MINOR 1

constexpr unsigned int ASSET_MAKER_VERSION = ROSE_VERSIONNUM(ASSET_MAKER_VERSION_MAJOR, ASSET_MAKER_VERSION_MINOR, 0);

namespace ROSE::AssetMaker {
  unsigned int GetVersion();
}
