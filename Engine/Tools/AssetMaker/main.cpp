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
#include <ROSE/Editor/ROSE_assetfile.h>

#include <cli/CLI.hpp>

#include <string>

using namespace ROSE;

using std::ifstream, std::ofstream;
using std::string;

#define ABORT_SHOW_HELP(returnval)                                                                                     \
  PrintF("{}\n", app.help());                                                                                          \
  return returnval

int main(int argc, char **argv) {

  std::string version = std::format("ROSE Asset Maker Version {}.{}\n\t"
                                    "\xC2\xA9 Viola Case, 2026. All right reserved.\n\n\t"
                                    "Designed for use with ROSE.",
                                    ASSET_MAKER_VERSION_MAJOR, ASSET_MAKER_VERSION_MINOR);
  CLI::App app {};
  app.set_version_flag("-i,-v,--version", version);

  string inputFile {};
  string typeStr {};
  string outputFile {};

  bool i;

  bool z;

  app.add_option("-f,--file", inputFile, "Specify the path of the file to pack");
  app.add_option("filename", inputFile);
  app.add_option("-t,--type", typeStr, "Specify the type");
  app.add_option("-o,--output", outputFile, "Specify the output file");
  app.add_flag("-z,--archive", z);


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
    PrintF("Failed to open {}", inputFile);
  }

  size_t size = std::filesystem::file_size(inputFile);

  auto *buf = new char[size] {};

  // copy file contents to buffer

  ifs.close();

  if (outputFile.empty()) {
    outputFile = inputFile;
  }

  outputFile = outputFile.substr(0, outputFile.find_last_of('.'));
  outputFile += ".roseasset";

  PrintF("Output file name: {}\n", outputFile);

  std::ofstream ofs(outputFile.c_str(), std::ios::binary);

  AssetFileHeader header;
  AssetType aType;
  if (typeStr.empty()) {
    string extension = inputFile.substr(inputFile.find_last_of('.') + 1);
    PrintF("Extension: {}\n", extension);

    aType = ParseAssetExtensionType(extension.c_str());
  } else if (typeStr.size() != 4) {
    PrintF("Extension must be 4 characters long!\n");
    delete[] buf;
    ABORT_SHOW_HELP(1);
  } else {
    char t[5]{};
    MemCpy(t, typeStr.c_str(), 4);
    aType = static_cast<AssetType>(Tag(t));
  }
  header.type = aType;

  ofs.close();

  delete[] buf;

  return 0;
}