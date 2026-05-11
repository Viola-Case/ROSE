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
#include "ROSE/Editor/ROSE_assetfile.h"

#include <cli/CLI.hpp>

#include <string>

using namespace ROSE;



int main(int argc, char **argv) {

  std::string version =
      std::format(
          "ROSE Asset Maker Version {}.{}\n\t"
          "\xC2\xA9 Viola Case, 2026. All right reserved.\n\n\t"
          "Designed for use with ROSE.",
          ASSET_MAKER_VERSION_MAJOR, ASSET_MAKER_VERSION_MINOR);
  CLI::App app {};
  app.set_version_flag("-i,-v,--version", version);

  std::string filename;
  std::string type;
  std::string output;

  bool i;

  bool z;

  app.add_option("-f,--file", filename, "Specify the path of the file to pack");
  app.add_option("filename", filename)->required(false);
  app.add_option("-t,--type", type, "Specify the type");
  app.add_option("-o,--output", output, "Specify the output file");
  app.add_flag("-z,--archive", z);


  app.name("ROSE-masset");

  CLI11_PARSE(app, argc, argv);

  if (argc == 1) {
    std::cout << app.help() << std::endl;
    return 0;
  }

  if (filename.empty()) {
    std::cout << app.help() << std::endl;
    return 0;
  }

  std::ifstream ifs(filename.c_str(), std::ios::binary);
  if (!ifs.is_open()) {
    PrintF("Failed to open {}", filename);
  }

  size_t size = std::filesystem::file_size(filename);

  char *buf = new char[size] {};
  // copy file contents to buffer
  ifs.close();

  if (output.empty()) {
    auto purefilename = filename.substr(0, filename.find_last_of('.'));
    output = purefilename + ".roseasset";
  }

  std::ofstream ofs(output.c_str(), std::ios::binary);


  ofs.close();

  delete[] buf;

  return 0;
}