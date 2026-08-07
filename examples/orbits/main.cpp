/**

    @file      main.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      07.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include <fstream>
#include <sstream>
#include <ROSE/ROSE.h>
using namespace ROSE;
int main() {


  List<Scene> scenes {};
  {
    std::ifstream ifs("assets/orbits.json");
    auto sstream = std::stringstream();
    sstream << ifs.rdbuf();
    String sceneJson = sstream.str();
    scenes.push_back({ Scene::FromJSONString(sceneJson) });
  }

  Application &app = *new Application(
    "Orbits",
    APPLICATION_SOFTWARE_RENDERER,
    Move(scenes),
    { 800, 800 }
    );
  {
    auto i = app.Init();
    if (i != 0) return i;
  }

  app.Run();

  delete &app;

  return 0;


}
