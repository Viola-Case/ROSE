#include <chrono>
#include <thread>
#include <ROSE/Editor/ROSE_editor.h>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

using namespace ROSE;

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
    ROSE::PrintF("\033[31mSDL init error:\033[0m\n\t{}", SDL_GetError());
  }

  SDL_Window *window = SDL_CreateWindow("ROSE Editor", 800, 600,
                                        reinterpret_cast<SDL_WindowFlags>(nullptr)
                                        // SDL_WINDOW_VULKAN | SDL_WINDOW_OPENGL
                                        | SDL_WINDOW_RESIZABLE
                                        | SDL_WINDOW_HIGH_PIXEL_DENSITY
                                        | SDL_WINDOW_BORDERLESS
  );

  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

  SDL_Renderer *renderer = SDL_CreateRenderer(window, "opengl");

  const bool *keys = SDL_GetKeyboardState(nullptr);

  auto start = std::chrono::high_resolution_clock::now();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;


  ImGui::StyleColorsClassic();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);


  while (true) {
    bool quit{false};
    {
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL3_ProcessEvent(&e);
        if (e.type == SDL_EVENT_QUIT) {
          quit = true;
        }
      }
      if (keys[SDL_SCANCODE_ESCAPE]) quit = true;
    }


    SDL_RenderClear(renderer);

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
      math::Vec2<int> windowPos{};

      SDL_GetWindowPosition(window, &windowPos.x, &windowPos.y);

      ImVec2 dragVec = ImGui::GetMouseDragDelta();

      windowPos += math::Vec2<int>(dragVec.x, dragVec.y);
      SDL_SetWindowPosition(window, windowPos.x, windowPos.y);

      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit")) quit = true;
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }
    if (quit) break;


    ImGui::Render();

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);


    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed_seconds = end - start;
    PrintF("{:.4f} seconds\n", elapsed_seconds.count());
    start = end;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }


  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
