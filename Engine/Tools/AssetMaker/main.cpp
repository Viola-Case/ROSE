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

  bool i;

  bool z;

  app.add_option("-f,--file", filename, "Specify the path of the file to pack");
  app.add_option("filename", filename)->required(false);
  app.add_option("-t,--type", type, "Specify the type");
  app.add_flag("-z,--archive", z);


  app.name("ROSE-masset");

  CLI11_PARSE(app, argc, argv);

  if (argc == 1) {
    std::cout << app.help() << std::endl;
    return 0;
  }

  if (filename.empty()) {
    // set filename to whatever field is unflagged
  }




  return 0;
}