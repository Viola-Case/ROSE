#include <ROSE/ROSE.h>

#include "applicationcloser.h"

using namespace ROSE;
int main() {

  BehaviorFactory::get().Register(MakeBehavior<AppCloser>, AppCloser::TypeID(), "Game1");



  auto scenes = List<Scene>();
  scenes.push_back(Scene::FromJSONString(
    R"(
{
  "name": "scene1",
  "objects": [
    {
      "name": "Scene Manager",
      "uuid": "aaaaaaaaaaaaaaaa-bbbbbbbbbbbbbbbb",
      "transform": {
        "position": [
          0,
          0,
          0
        ],
        "rotation": [
          0,
          0,
          0
        ],
        "scale": [
          1,
          1,
          1
        ]
      },
      "behaviors": [
        {
          "typeid": "1510c09900c8cc39-21a67c5c20659851",
          "factoryParameters": []
        }
      ]
    }
  ]
}
)"
    ));



  Application Game("Game 1", 0, Move(scenes));



  Game.Init();



  Game.Run();


}