/**

  @file       ROSE_asset.h
  @brief
  @details    ~
  @author     Viola Case
  @date       05.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_fileformat.h>
#include <ROSE/Core/ROSE_version.h>
#include <ROSE/Core/ROSE_asset.h>



namespace ROSE {

  /**
    [4]  magic       "ROSE"   \n
    [4]  file_kind   "ASET" (or whatever)   \n
    [2]  version major    \n
    [2]  version minor    \n
    [4]  asset_type   \n
    [4]  flags    \n
    [4]  metadata_size    \n
    [4]  data_size    \n
    [N]  metadata   \n
    [M]  data   \n
   */
  struct AssetFileHeader {
    FileHeader header { FileType::Asset };

    uint16_t versionMajor { ROSE_VERSION_MAJOR };
    uint16_t versionMinor { ROSE_VERSION_MINOR };
    // no patch version because it's probably not going to change asset handling (and if a patch does, it'll change
    // the minor version number or I quit software development)

    AssetType type;

    AssetFlags flags;

    uint32_t metaDataSize; //!< Bytes
    uint32_t dataSize; //!< Bytes

  };

  struct AssetFile {
    AssetFileHeader header;
    char *metaData;
    char *data;
  };


} // namespace ROSE
