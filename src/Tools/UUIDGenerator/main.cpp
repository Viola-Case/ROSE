/**

  @file       main.cpp
  @brief      
  @details    ~
  @author     Viola Case
  @date       27.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/

#include <ROSE/ROSE.h>
#include <cstdio>
#include <CLI/CLI.hpp>

constexpr int UUID_GENERATOR_VERSION_MAJOR = 0;
constexpr int UUID_GENERATOR_VERSION_MINOR = 1;

int main(int argc, char **argv) {
  CLI::App app;
  std::string version {std::format(
    "ROSE UUID Generator version {}.{}\n\t"
    "© Viola Case, 2026. All right reserved.\n\n\t"
    "Designed for use with ROSE.",
    UUID_GENERATOR_VERSION_MAJOR, UUID_GENERATOR_VERSION_MINOR)};
  app.set_version_flag("-v", version);

  app.set_help_flag();

  CLI11_PARSE(app, argc, argv);

  auto uuid = ROSE::UUID::Generate();
  ROSE::PrintF("{:x}-{:x}", uuid.high, uuid.low);

  return 0;

}