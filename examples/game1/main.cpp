#include <ROSE/ROSE.h>

using namespace ROSE;
int main() {


  auto scenes = List<Scene>();

  //scenes.push_back(Scene());

  Application Game("Game 1", 0, Move(scenes));

  Game.Init();



  Game.Run();


}