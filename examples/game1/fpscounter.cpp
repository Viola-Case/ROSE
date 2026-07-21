/**

  @file       fpscounter.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       19.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#define ROSE_LOGLEVELS_USE_NERDFONT_SYMBOLS
#include "fpscounter.h"
#include <ROSE/Core/time.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
constexpr float fontSize { 18.f };
constexpr double refreshInterval { 0.25 }; //!< seconds between texture rebuilds
constexpr const char *fontPath { "assets/font.ttf" };

using namespace ROSE;

FpsCounter::~FpsCounter() {
  if (m_texture) SDL_DestroyTexture(m_texture);
  if (m_font) TTF_CloseFont(m_font);
  if (m_ttfInited) TTF_Quit();
}

void FpsCounter::OnStart() {
  auto window = static_cast<SDL_Window *>(GetObject().GetScene().GetApplication().GetWindow());
  m_renderer = SDL_GetRenderer(window);

  if (!TTF_Init()) {
    Log(LogLevel::Error, "TTF_Init failed: {}", SDL_GetError());
    return;
  }
  m_ttfInited = true;

  m_font = TTF_OpenFont(fontPath, fontSize);
  if (!m_font)
    Log(LogLevel::Error, "Failed to open '{}': {}", fontPath, SDL_GetError());
}

void FpsCounter::FrameUpdate() {
  ROSE_LOG_INFO("Time::deltaTime: {}\n", Time::deltaTime);

  if (!m_renderer || !m_font) return;

  m_accum += Time::deltaTime;
  if (m_accum >= refreshInterval && Time::deltaTime > 0.) {
    RebuildTexture(1. / Time::deltaTime);
    m_accum = 0.;
  }

  if (!m_texture) return;
  const auto &pos = m_object->transform.position;
  const SDL_FRect dst {
    static_cast<float>(pos.x),
    static_cast<float>(pos.y),
    m_textW,
    m_textH
  };
  SDL_RenderTexture(m_renderer, m_texture, nullptr, &dst);
}

void FpsCounter::RebuildTexture(double fps) {
  char text[32];
  SDL_snprintf(text, sizeof text, "%.0f FPS", fps);

  constexpr SDL_Color white { 255, 255, 255, 255 };
  SDL_Surface *surface = TTF_RenderText_Blended(m_font, text, 0, white);
  if (!surface) return;

  if (m_texture) SDL_DestroyTexture(m_texture);
  m_texture = SDL_CreateTextureFromSurface(m_renderer, surface);
  m_textW = static_cast<float>(surface->w);
  m_textH = static_cast<float>(surface->h);
  SDL_DestroySurface(surface);
}
