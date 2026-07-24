/**

  @file       ball.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       22.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "ball.h"
#include "paddle.h"
#include <cmath>
#include <random>
#include <SDL3/SDL.h>

using namespace ROSE;

constexpr float size { 10 };
constexpr double speed { 300. };


namespace {
  int w {}, h {};
}

void Ball::Reset() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_real_distribution<> dis(1. / 8. * math::PI, 7. / 8. * math::PI);
  static std::uniform_int_distribution<> intdis(0, 127);

  auto &pos = m_object->transform.position;

  pos.x = std::uniform_real_distribution<> { 0, (double)w }(gen);
  pos.y = h / 2;

  auto angle = dis(gen);
  auto dir = intdis(gen) % 2 == 0 ? -1. : 1.;

  m_motion = m_object->FindBehavior<Motion>();
  if (!m_motion) {
    ROSE_LOG_ERROR("Motion object not found!");
    return;
  }
  Vec3d v { math::Cos(angle) * dir, math::Sin(angle) * dir, 0 };
  m_motion->SetVelocity(v * speed);
}

void Ball::OnStart() {
  auto window = static_cast<SDL_Window *>(m_object->GetScene().GetApplication().GetWindow());
  m_renderer = SDL_GetRenderer(window);
  SDL_GetWindowSize(window, &w, &h);

  this->Reset();

  auto &scene = m_object->GetScene();
  m_paddles[0] = scene.FindObjectByName("Paddle1");
  m_paddles[1] = scene.FindObjectByName("Paddle2");
}

void Ball::FrameUpdate() {
  auto &pos = m_object->transform.position;

  if (m_motion) {

    if (InputSystem::GetKeyDown(KeyCode::R))
      this->Reset();

    auto &v = m_motion->GetVelocity();
    if (v.x > 0 && pos.x >= w || v.x < 0 && pos.x <= 0) {
      v.x *= -1;
    }

    constexpr float ballHalf { size * 0.5f };
    constexpr float paddleHalfW { Paddle::width * 0.5f };
    constexpr float paddleHalfH { Paddle::height * 0.5f };
    for (auto *paddle : m_paddles) {
      if (!paddle) continue;
      const auto &ppos = paddle->transform.position;
      const bool overlap =
        std::abs(pos.x - ppos.x) <= paddleHalfW + ballHalf && std::abs(pos.y - ppos.y) <= paddleHalfH + ballHalf;
      if (!overlap) continue;
      /* Bounce the ball away from the paddle: if the ball is above the
       * paddle it must travel up, if below it must travel down. Only flip
       * when the ball is actually heading into the paddle so it can't stick. */
      if (pos.y < ppos.y && v.y > 0) v.y *= -1;
      else if (pos.y > ppos.y && v.y < 0) v.y *= -1;
    }
  }

  if (!m_renderer) {
    return;
  }
  auto renderer = static_cast<SDL_Renderer *>(m_renderer);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
  const SDL_FRect rect { static_cast<float>(pos.x) - size * 0.5f, static_cast<float>(pos.y) - size * 0.5f, size, size };
  SDL_RenderFillRect(renderer, &rect);
}
