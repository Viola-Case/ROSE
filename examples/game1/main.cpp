#include <ROSE/ROSE.h>

using namespace ROSE;
int main() {


  auto scenes = List<Scene>();

  Application &Game = *(new Application("Game 1", 0, Move(scenes)));

  Game.Init();



  Game.Run();

  delete &Game;


}