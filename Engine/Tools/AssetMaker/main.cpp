/**

    @file      main.cpp
    @brief     
    @details   ~
    @author    Viola Case
    @date      29.04.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include <nlohmann/json.hpp>

#include <ROSE/ROSE.h>

using namespace ROSE;
using namespace nlohmann;
using namespace nlohmann::literals;

String json_test {R"({
"funny":true
}
)"};

int main() {
  nlohmann::json testJSON = nlohmann::json::parse(json_test.c_str());
  PrintF("{}",testJSON["funny"].get<bool>());
  return 0;
}