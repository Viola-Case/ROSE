/**

    @file      main.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      29.04.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/ROSE.h>

#include "assetmaker_version.h"
#include "assetmaker_input.h"
#include <ROSE/Editor/assetfile.h>

#include <cli/CLI.hpp>

#include <string>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

using namespace ROSE;

using std::ifstream, std::ofstream;
using std::string;

#define ABORT_SHOW_HELP(returnval)                                                                                     \
  PrintF("{}\n", app.help());                                                                                          \
  return returnval

// TODO rewrite this as a library


int main(int argc, char **argv) {

  // TODO figure out what's wrong with the copyright sign
  std::string version = std::format("ROSE Asset Maker Version {}.{}\n\t"
                                    "© Viola Case, 2026. All right reserved.\n\n\t"
                                    "Designed for use with ROSE.",
                                    ASSET_MAKER_VERSION_MAJOR, ASSET_MAKER_VERSION_MINOR);
  CLI::App app {};
  app.set_version_flag("-i,-v,--version", version);

  string inputFile {};
  string typeStr {};
  string outputFile {};

  // bool z;

  app.add_option("-f,--file", inputFile, "Specify the path of the file to pack");
  app.add_option("filename", inputFile);
  app.add_option("-t,--type", typeStr, "Specify the type");
  app.add_option("-o,--output", outputFile, "Specify the output file");
  // app.add_flag("-z,--archive", z);


  app.name("ROSE-masset");

  CLI11_PARSE(app, argc, argv);

  if (argc == 1) {
    ABORT_SHOW_HELP(0);
  }

  if (inputFile.empty()) {
    ABORT_SHOW_HELP(0);
  }

  std::ifstream ifs(inputFile.c_str(), std::ios::binary);
  if (!ifs.is_open()) {
    std::cerr << Format("Failed to open {}", inputFile).c_str();
    ABORT_SHOW_HELP(1);
  }

  size_t size = std::filesystem::file_size(inputFile);

  RawBuffer buf;

  //ifs.read(buf.data(), size);

  //ifs.close();

  if (outputFile.empty()) {
    outputFile = inputFile;
    outputFile = outputFile.substr(0, outputFile.find_last_of('.'));
    outputFile += ".roseasset";
    PrintF("No output file specified, setting output file to {}\n", outputFile);

  } else {
    outputFile = outputFile.substr(0, outputFile.find_last_of('.'));
    outputFile += ".roseasset";
  }

  AssetFileHeader header;
  AssetType aType;
  if (typeStr.empty()) {

    PrintF("No asset type specified, parsing file extension for implicit type...\n");
    string extension = inputFile.substr(inputFile.find_last_of('.') + 1);
    aType = ParseAssetExtensionType(extension.c_str());
    auto typeTag = uint32_t(aType);
    PrintF("Type inferred: {}{:c}{:c}{:c}{:c}{}\n", "\033[36m", (typeTag >> 0) & 0xff, (typeTag >> 8) & 0xff,
           (typeTag >> 16) & 0xff, (typeTag >> 24) & 0xff, "\033[0m");

  } else if (typeStr.size() != 4) {

    PrintF("{}Extension must be 4 characters long!{}\n", "\033[33m", "\033[0m");
    ABORT_SHOW_HELP(1);

  } else {

    char t[5] {};
    MemCpy(t, typeStr.c_str(), 4);
    aType = static_cast<AssetType>(Tag(t));
  }

  header.type = aType;
  header.dataSize = size;

  std::ofstream ofs(outputFile.c_str(), std::ios::binary);
  if (!ofs.is_open()) {
    PrintF("{}ERROR: {} Failed to open file for writing!", "\033[31m", "\033[0m");
  }

  ofs.write(reinterpret_cast<char *>(&header), sizeof(header));

  // TODO write texture data properly as SDL surface instead of copying file contents

  if (aType == AssetType::Texture) {
    header.metaDataSize = 0;
    SDL_Surface *surf = IMG_Load(inputFile.c_str());
    void *pix = surf->pixels;
    const size_t pixelBytes = SDL_GetPixelFormatDetails(surf->format)->bytes_per_pixel * surf->pitch * surf->h;
    buf.allocate(pixelBytes);
    MemCpy(buf.data(), pix, pixelBytes);
  }

  //ofs.write(buf.data(), buf.size());

  ofs.close();

  PrintF("Wrote to {}\n", outputFile);

  return 0;
}