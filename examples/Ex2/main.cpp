#include <ROSE/ROSE.h>

#include <vector>

#include <iostream>
#include <cstdint>
#include <chrono>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

using namespace ROSE;

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

void DrawNgon(SDL_Renderer *renderer, float cx, float cy, float radius, int n, SDL_FColor color) {
  std::vector<SDL_Vertex> verts;

  for (int i = 0; i < n; i++) {
    float a0 = (2.0f * math::PI / n) * i;
    float a1 = (2.0f * math::PI / n) * (i + 1);

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

  bool TTF_FAILED{ 0 };
  if (!TTF_Init()) TTF_FAILED = true;

  TTF_Font *font = TTF_OpenFont("C:/Windows/Fonts/CalibriL.ttf",24);

  SDL_Window *window = SDL_CreateWindow("window", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_BORDERLESS);

  InputSystem::Prime();

  /*SDL_JoystickID *sticks = SDL_GetJoysticks(nullptr);
  if (!sticks) {
    std::wcout << L"SDL joysticks problem: " << SDL_GetError() << std::endl;
    return -1;
  }

  SDL_Gamepad *gamepad = nullptr;

  for (size_t i = 0; sticks[i] != 0; ++i) {
    auto j = sticks[i];
    if (SDL_IsGamepad(j)) gamepad = SDL_OpenGamepad(j);
  }
  SDL_free(sticks);*/

  const bool *keys = SDL_GetKeyboardState(nullptr);

  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "3");
  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsClassic();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  //String gamepadName = SDL_GetGamepadName(gamepad);
  String gamepadName = InputSystem::GetGamepadName();

  std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();
  SDL_Texture *texture{nullptr};
  while (true) {
    auto dur = std::chrono::high_resolution_clock::now() - start;
    start = std::chrono::high_resolution_clock::now();

    String time = Format("FPS: {:.1f}", 1 / (std::chrono::duration_cast<std::chrono::duration<float>>(dur).count()));
    

    

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
    
    static int c = 0;
    c += 1;
    c = c % 10;
    if (c == 0) {
      if (texture) SDL_DestroyTexture(texture);
      SDL_Color color = { 255, 255, 255, 255 };
      SDL_Surface *surface = TTF_RenderText_Blended(font, time.c_str(), 0, color);
      texture = SDL_CreateTextureFromSurface(renderer, surface);
      SDL_DestroySurface(surface); // done with this now
      std::cout << time.c_str() << std::endl;
    }

    SDL_SetRenderDrawColor(renderer, 10, 30, 80, 255);
    SDL_RenderClear(renderer);

    

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    float w{ 50 }, h{ 10 };
    SDL_GetTextureSize(texture, &w, &h);
    SDL_FRect dst = { 685, 15, w, h };
    SDL_RenderTexture(renderer, texture, nullptr, &dst);


    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();

    ImGuiWindowFlags titlebar_flags =
      ImGuiWindowFlags_NoDecoration |
      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize({ (float) WINDOW_WIDTH, 30.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    if (ImGui::Begin("##titlebar", nullptr, titlebar_flags)) {
      ImGui::Text(SDL_GetWindowTitle(window));
      // buttons will go here
      ImGui::SameLine(WINDOW_WIDTH - 30.0f);
      if (ImGui::Button("X", { 20.0f, 20.0f })) {
        quit = true;
      }
      ImGui::End();
      if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(0)) {
        ImVec2 delta = ImGui::GetMouseDragDelta();
        math::Vec2<int> windowPos;
        SDL_GetWindowPosition(window, &(windowPos.x), &(windowPos.y));
        SDL_SetWindowPosition(window, windowPos.x + delta.x, windowPos.y + delta.y);
      }
    }
    ImGui::PopStyleVar();
    
    //if (ImGui::BeginMainMenuBar()) {
    //
    //  if (ImGui::BeginMenu("File")) {
    //    if (ImGui::MenuItem("Exit")) quit = true;
    //    ImGui::EndMenu();
    //  }
    //  ImGui::EndMainMenuBar();
    //
    //}

    if (keys[SDL_SCANCODE_ESCAPE]) quit = true;

    //ImGui_ImplSDL3_NewFrame();
    //ImGui::NewFrame();

    Vec2f GamepadLeft{InputSystem::GetStickAxes(GamepadStick::Left)};
    Vec2f GamepadRight{ InputSystem::GetStickAxes(GamepadStick::Right) };
    
    float GamepadTriggerLeft = InputSystem::GetGamepadAxis(GamepadAxis::LeftTrigger);
    float GamepadTriggerRight = InputSystem::GetGamepadAxis(GamepadAxis::RightTrigger);

    ImGui::Begin("Gamepad Info", nullptr, 0);
    ImGui::Text("%s", gamepadName.c_str());
    ImGui::Text("Left X: %f", GamepadLeft.x);
    ImGui::Text("Left Y: %f", GamepadLeft.y);
    ImGui::Text("Right X: %f", GamepadRight.x);
    ImGui::Text("Right Y: %f", GamepadRight.y);
    ImGui::Text("Trigger Left: %f", GamepadTriggerLeft);
    ImGui::Text("Trigger Right: %f", GamepadTriggerRight);
    ImGui::Text("Start: %d", InputSystem::GetGamepadButton(GamepadButton::START));
    ImGui::Text("Back: %d", InputSystem::GetGamepadButton(GamepadButton::BACK));
    ImGui::End();
    
    ImGui::Render();


    //SDL_SetRenderDrawColor(renderer, 40, 120, 240, 255);
    //SDL_FRect rects[2]{ { 80,280,240,240 },{ 480,280,240,240 } };
    
    DrawNgon(renderer, WINDOW_WIDTH / 4, WINDOW_WIDTH / 2, 100, 100, { 20 / 255.f, 60 / 255.f, 120 / 255.f, 255 / 255.f });
    DrawNgon(renderer, WINDOW_WIDTH / 4 * 3, WINDOW_WIDTH / 2, 100, 100, { 20 / 255.f, 60 / 255.f, 120 / 255.f, 255 / 255.f });

    DrawNgon(renderer, WINDOW_WIDTH / 4 + (100 * GamepadLeft.x), WINDOW_WIDTH / 2 + (100 * GamepadLeft.y), 20, 20, { 40/255.f, 120/255.f, 240/255.f, 255 });
    DrawNgon(renderer, 3 * WINDOW_WIDTH / 4 + (100 * GamepadRight.x), WINDOW_WIDTH / 2 + (100 * GamepadRight.y), 20, 20, { 40/255.f, 120/255.f, 240/255.f, 255 });

    
    //SDL_RenderRects(renderer, rects, 2);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);

    if (quit) break;
  }

  TTF_CloseFont(font);

  SDL_Quit();

  return 0;
}