#include <ROSE/ROSE.h>

#include "applicationcloser.h"
#include "paddle.h"

#include <fstream>
#include <sstream>

using namespace ROSE;
int main() {

  BehaviorFactory::get().Register(MakeBehavior<AppCloser>, AppCloser::TypeID(), "Game1");
  BehaviorFactory::get().Register(MakeBehavior<Paddle>, Paddle::TypeID(), "Game1");


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

  Game.Init();

  Game.Run();


}