/**

  @file       AMversion.cpp
  @brief      
  @details    ~
  @author     Viola Case
  @date       10.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/

#include "assetmaker_version.h"

namespace ROSE::AssetMaker {
  unsigned int GetVersion() {
    return ASSET_MAKER_VERSION;
  }
}