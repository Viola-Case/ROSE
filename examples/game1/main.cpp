#include <ROSE/ROSE.h>

#include "applicationcloser.h"
#include "fpscounter.h"
#include "paddle.h"

#include <fstream>
#include <sstream>

using namespace ROSE;
int main() {

  {
    BehaviorFactory &factory = BehaviorFactory::get();
    Pair<FactoryFn, UUID> fns[] {
      { MakeBehavior<AppCloser>, AppCloser::TypeID() },
      { MakeBehavior<Paddle>, Paddle::TypeID() },
      { MakeBehavior<FpsCounter>, FpsCounter::TypeID() }
    };
    for (const auto &p : fns)
      factory.Register(p.first, p.second, "Game1");
  }



  auto scenes = List<Scene>();

  String sceneJSON;
  {
    std::ifstream sceneJSONStream("assets/game1scene1.json");
    auto sstream = std::stringstream();
    sstream << sceneJSONStream.rdbuf();
    sceneJSON = sstream.str();
  }
  scenes.push_back(Scene::FromJSONString(sceneJSON));

  Application Game("Game 1", 0, Move(scenes));
  Game.SetFlag(ApplicationFlag::SoftwareRenderer, 1);

  Game.Init();

  Game.Run();
}