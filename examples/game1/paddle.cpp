/**

  @file       paddle.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       15.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "paddle.h"
#include <ROSE/Core/time.h>
#include <SDL3/SDL.h>

constexpr double speed { 500 };
constexpr float width { 120.f };
constexpr float height { 24.f };
constexpr float margin { 24.f }; //!< gap between a paddle's center and its screen edge

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

void Paddle::OnStart() {
  auto window = static_cast<SDL_Window *>(GetObject().GetScene().GetApplication().GetWindow());
  SDL_GetWindowSize(window, &m_screenW, &m_screenH);
  m_renderer = SDL_GetRenderer(window);

  auto &pos = m_object->transform.position;
  pos.x = m_screenW * 0.5;
  pos.y = m_player == P1 ? m_screenH - margin : margin;
}

void Paddle::FrameUpdate() {
  auto &pos = m_object->transform.position;

  double dir {};
  if (InputSystem::GetKey(m_keyLeft)) dir -= 1.;
  if (InputSystem::GetKey(m_keyRight)) dir += 1.;
  pos.x += dir * speed * Time::deltaTime;

  constexpr double half { width * 0.5 };
  if (pos.x < half) pos.x = half;
  if (pos.x > m_screenW - half) pos.x = m_screenW - half;

  if (!m_renderer) return;
  SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
  const SDL_FRect rect {
    static_cast<float>(pos.x) - width * 0.5f,
    static_cast<float>(pos.y) - height * 0.5f,
    width,
    height
  };
  SDL_RenderFillRect(m_renderer, &rect);
}
