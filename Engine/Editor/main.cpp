#include <ROSE/Editor/ROSE_editor.h>
#include <SDL3/SDL.h>

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {}
}