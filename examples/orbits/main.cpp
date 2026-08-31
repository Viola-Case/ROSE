/**

    @file      main.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      07.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/



#include <ROSE/ROSE.h>
#include <ROSE/Core/imgui.h>

#include "closer.h"
#include "pointcloud.h"

constexpr int WIDTH = 800, HEIGHT = 800;

using namespace ROSE;
using namespace Orbits;

int main() {
  if (auto i = Init(); i != InitStatus::Success) {
    return static_cast<int>(i);
  }

  {
    BehaviorFactory &factory = BehaviorFactory::Get();
    RoseRegisterCoreModule(factory);
    Pair<FactoryFn, UUID> fns[] {
      ROSE_BEHAVIOR_REGISTRY_PAIR(PointCloud),
      ROSE_BEHAVIOR_REGISTRY_PAIR(Closer),
    };
    for (const auto &p : fns) {
      factory.Register(p.first, p.second, "Orbits");
    }
  }

  ApplicationInitSettings settings { "Orbits" };
  settings.SetFlags(APPLICATION_SDL_RENDERER)
    .SetWindowSize(WIDTH, HEIGHT)
    .AddSceneFromFile("assets/orbits.json")
    .SetVSync(false);

  Application app;
  if (const int err = app.Init(Move(settings))) return err;

  AttachImGui();

  app.Run();

  return 0;
}
