#include <ROSE/ROSE.h>

using namespace ROSE;
int main() {


  Application &Game = *(new Application("Game 1"));
  ApplicationManager::LinkApplication(Game);



}