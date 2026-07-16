/**

  @file       paddle.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       15.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "paddle.h"
#include <SDL3/SDL.h>

constexpr double speed { 50. };
constexpr int width { 50 };

using namespace ROSE;
Paddle::Paddle() noexcept : m_keyLeft(KeyCode::LEFT), m_keyRight(KeyCode::RIGHT), m_player(P1) {}

void Paddle::Unpack(const ParamView &view) {
  m_player = static_cast<Paddle::Player>(view.GetInt("player", P1));
}

void Paddle::OnCreate() {
  PrintF("Paddle {} created\n", m_player == P1 ? "1" : "2");
  switch (m_player) {
  case P1:
    break;
  case P2:
    m_keyLeft = KeyCode::A;
    m_keyRight = KeyCode::D;
    break;
  }
}

void Paddle::FrameUpdate() {
  static int screenWidth {};
  if (!screenWidth) {
    auto w = static_cast<SDL_Window *>(GetObject().GetScene().GetApplication().GetWindow());
    int h;
    SDL_GetWindowSize(w, &screenWidth, &h);
  }
}