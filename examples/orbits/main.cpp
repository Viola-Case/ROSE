/**

    @file      main.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      07.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/



#include <ROSE/ROSE.h>

constexpr int WIDTH = 800, HEIGHT = 800;

using namespace ROSE;
int main() {
  if (auto i = Init(); i != InitStatus::Success) {
    return static_cast<int>(i);
  }
  {
    BehaviorFactory &factory = BehaviorFactory::Get();
    RoseRegisterCoreModule(factory);
  }

  ApplicationInitSettings settings { "Orbits" };
  settings.SetFlags(APPLICATION_SOFTWARE_RENDERER).SetWindowSize(WIDTH, HEIGHT).AddSceneFromFile("assets/orbits.json");

  Application app;
  if (const int err = app.Init(Move(settings))) return err;

  app.Run();

  return 0;
}
