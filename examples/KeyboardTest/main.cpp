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
#include <SDL3_image/SDL_image.h>

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

  String path;
#if defined(_WIN32)
  path = "C:/Windows/Fonts/CalibriL.ttf";
#elif defined(__linux__)
  path = "/usr/share/fonts/gnu-free/FreeMono.otf";
#endif


  TTF_Font *font = TTF_OpenFont(path.c_str(),24);

  SDL_Window *window = SDL_CreateWindow("window", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIGH_PIXEL_DENSITY);



  String PATH = SDL_GetBasePath();
  SDL_Surface* ROSE_LOGO_SURFACE;

  {
    String ICON_PATH;
    ICON_PATH.reserve(128);
    ICON_PATH = PATH;
    ICON_PATH += "rose_icon.png";
    std::wcout << ICON_PATH.c_str() << std::endl;
    ROSE_LOGO_SURFACE = IMG_Load(ICON_PATH.c_str());
    if (!ROSE_LOGO_SURFACE)
      std::wcout << "problem: " << SDL_GetError() << std::endl;
  }

  if (!SDL_SetWindowIcon(window,ROSE_LOGO_SURFACE))
    std::wcout << "problem: " << SDL_GetError() << std::endl;

  //getchar();

  InputSystem::Prime();

  const bool *keys = SDL_GetKeyboardState(nullptr);

  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsClassic();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

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

    float lx = -InputSystem::GetKey(KeyCode::A) + InputSystem::GetKey(KeyCode::D);
    float ly = -InputSystem::GetKey(KeyCode::S) + InputSystem::GetKey(KeyCode::W);
    float rx = -InputSystem::GetKey(KeyCode::LEFT) + InputSystem::GetKey(KeyCode::RIGHT);
    float ry = -InputSystem::GetKey(KeyCode::DOWN) + InputSystem::GetKey(KeyCode::UP);

    ImGui::Begin("Keyboard Info", nullptr, 0);
    ImGui::Text("Left X: %f", lx);
    ImGui::Text("Left Y: %f", ly);
    ImGui::Text("Right X: %f", rx);
    ImGui::Text("Right Y: %f", ry);
    ImGui::End();
    
    ImGui::Render();


    //SDL_SetRenderDrawColor(renderer, 40, 120, 240, 255);
    //SDL_FRect rects[2]{ { 80,280,240,240 },{ 480,280,240,240 } };
    
    DrawNgon(renderer, WINDOW_WIDTH / 4, WINDOW_WIDTH / 2, 100, 100, { 20 / 255.f, 60 / 255.f, 120 / 255.f, 255 / 255.f });
    DrawNgon(renderer, WINDOW_WIDTH / 4 * 3, WINDOW_WIDTH / 2, 100, 100, { 20 / 255.f, 60 / 255.f, 120 / 255.f, 255 / 255.f });

    DrawNgon(renderer, WINDOW_WIDTH / 4 + (100 * lx), WINDOW_WIDTH / 2 + (100 * -ly), 20, 20, { 40/255.f, 120/255.f, 240/255.f, 255 });
    DrawNgon(renderer, 3 * WINDOW_WIDTH / 4 + (100 * rx), WINDOW_WIDTH / 2 + (100 * -ry), 20, 20, { 40/255.f, 120/255.f, 240/255.f, 255 });

    
    //SDL_RenderRects(renderer, rects, 2);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);

    if (quit) break;
  }

  TTF_CloseFont(font);

  SDL_Quit();

  return 0;
}