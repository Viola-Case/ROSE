#include <ROSE/ROSE.h>
#include <ROSE/Core/imgui.h>

#include "applicationcloser.h"
#include "fpscounter.h"
#include "paddle.h"
#include "ball.h"
#include "scoreboard.h"

using namespace ROSE;
int main() {

  if (auto i = Init(); i != InitStatus::Success) {
    return static_cast<int>(i);
  }

  {
    BehaviorFactory &factory = BehaviorFactory::Get();
    RoseRegisterCoreModule(factory);
    Pair<FactoryFn, UUID> fns[] {
      ROSE_BEHAVIOR_REGISTRY_PAIR(AppCloser), ROSE_BEHAVIOR_REGISTRY_PAIR(Paddle),
      ROSE_BEHAVIOR_REGISTRY_PAIR(Scoreboard), ROSE_BEHAVIOR_REGISTRY_PAIR(Ball),
      ROSE_BEHAVIOR_REGISTRY_PAIR(FpsCounter), ROSE_BEHAVIOR_REGISTRY_PAIR(ResetController),
    };
    for (const auto &p : fns)
      factory.Register(p.first, p.second, "Game1");
  }

  ApplicationInitSettings settings { "Game 1" };
  settings.SetFlags(APPLICATION_SOFTWARE_RENDERER)
    .SetWindowSize(800, 600)
    .AddSceneFromFile("assets/game1scene1.json")
    .SetVSync(true);

  Application Game;
  if (const int err = Game.Init(Move(settings))) return err;

  // FpsCounter draws with ImGui from this executable, which links its own copy of
  // ImGui. Point it at the context Init() just created inside ROSE_Core.dll.
  AttachImGui();

  Game.Run();

  return 0;
}