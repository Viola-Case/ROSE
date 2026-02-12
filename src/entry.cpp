/**

  @file      main.cpp
  @brief     
  @details   ~
  @author    Cool Guy
  @date      12.02.2026
  @copyright © Cool Guy, 2026. All right reserved.

**/

#include <ROSE/ROSE.h>

#include <Windows.h>


#if !defined(_DEBUG) || defined(_NDEBUG)

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
  int argc = __argc;
  char **argv = __argv;
  return ROSE_main(argc, argv);
}

#else

int main(int argc, char **argv) {
  return ROSE_main(argc, argv);
}

#endif // !ROSE_MAIN_HANDLED
