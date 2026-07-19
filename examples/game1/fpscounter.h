/**

  @file       fpscounter.h
  @brief
  @details    ~
  @author     Viola Case
  @date       19.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/ROSE.h>

struct SDL_Renderer;
struct SDL_Texture;
struct TTF_Font;

using namespace ROSE;

class FpsCounter : public Behavior {
public:
  ~FpsCounter() override;
  static constexpr UUID typeID = "995243d320724db5-5fabb4eb31659861"_uuid;
  static constexpr UUID TypeID() noexcept { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }

protected:
  void OnStart() override;
  void FrameUpdate() override;

private:
  void RebuildTexture(double fps);

  SDL_Renderer *m_renderer {};
  TTF_Font *m_font {};
  SDL_Texture *m_texture {};
  float m_textW {};
  float m_textH {};
  double m_accum {};
  bool m_ttfInited {};
};
