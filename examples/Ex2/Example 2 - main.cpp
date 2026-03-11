#include <ROSE/ROSE.h>

#include <vector>

#include <iostream>
#include <cstdint>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <SDL3/SDL.h>

constexpr float PI = 3.14159265358979323846f;

using namespace ROSE;

void DrawNgon(SDL_Renderer *renderer, float cx, float cy, float radius, int n, SDL_FColor color) {
  std::vector<SDL_Vertex> verts;

  for (int i = 0; i < n; i++) {
    float a0 = (2.0f * PI / n) * i;
    float a1 = (2.0f * PI / n) * (i + 1);

    SDL_Vertex v0 = { { cx, cy },                                         color, {0,0} };
    SDL_Vertex v1 = { { cx + radius * cosf(a0), cy + sinf(a0) * radius }, color, {0,0} };
    SDL_Vertex v2 = { { cx + radius * cosf(a1), cy + sinf(a1) * radius }, color, {0,0} };

    verts.insert(verts.end(), { v0, v1, v2 });
  }

  SDL_RenderGeometry(renderer, nullptr, verts.data(), verts.size(), nullptr, 0);
}

int main(int argc, char **argv) {
  
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS)) {
    std::wcout << L"SDL init problem: " << SDL_GetError() << std::endl;
    return -1;
  }

  SDL_Window *window = SDL_CreateWindow("window", 800, 800, NULL);

  SDL_JoystickID *sticks = SDL_GetJoysticks(nullptr);
  if (!sticks) {
    std::wcout << L"SDL joysticks problem: " << SDL_GetError() << std::endl;
    return -1;
  }

  SDL_Gamepad *gamepad = nullptr;

  for (size_t i = 0; sticks[i] != 0; ++i) {
    auto j = sticks[i];
    if (SDL_IsGamepad(j)) gamepad = SDL_OpenGamepad(j);
  }

  const bool *keys = SDL_GetKeyboardState(nullptr);

  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  while (true) {

    bool quit{ false };
    {
      
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL3_ProcessEvent(&e);
        if (e.type == SDL_EVENT_QUIT)
          quit = true;
      }
      //system("cls");
      //std::wcout << "Left X:" << SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) << std::endl;
      //std::wcout << "Left Y:" << SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) << std::endl;
      //std::wcout << "Right X:" << SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX) << std::endl;
      //std::wcout << "Right Y:" << SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY) << std::endl;

    }
    SDL_SetRenderDrawColor(renderer, 10, 30, 80, 255);
    SDL_RenderClear(renderer);

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();

    if (keys[SDL_SCANCODE_ESCAPE]) quit = true;

    //ImGui_ImplSDL3_NewFrame();
    //ImGui::NewFrame();

    Vec2f GamepadLeft(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) / 32768., SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) / 32768.);
    Vec2f GamepadRight(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX) / 32768., SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY) / 32768.);
    
    ImGui::Begin("Gamepad Info", nullptr, 0);
    ImGui::Text("Left X: %f", GamepadLeft.x);
    ImGui::Text("Left Y: %f", GamepadLeft.y);
    ImGui::Text("Right X: %f", GamepadRight.x);
    ImGui::Text("Right Y: %f", GamepadRight.y);
    ImGui::End();
    
    ImGui::Render();

    SDL_SetRenderDrawColor(renderer, 40, 120, 240, 255);
    SDL_FRect rects[2]{ { 80,280,240,240 },{ 480,280,240,240 } };
    
    DrawNgon(renderer, 200 + (100 * GamepadLeft.x), 400 + (100 * GamepadLeft.y), 20, 20, { 40/255., 120/255., 240/255., 255 });
    DrawNgon(renderer, 600 + (100 * GamepadRight.x), 400 + (100 * GamepadRight.y), 20, 20, { 40/255., 120/255., 240/255., 255 });

    
    SDL_RenderRects(renderer, rects, 2);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);

    if (quit) break;
  }


  SDL_Quit();

  return 0;
}