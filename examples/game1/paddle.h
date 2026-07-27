/**

  @file       paddle.h
  @brief
  @details    ~
  @author     Viola Case
  @date       15.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/ROSE.h>

struct SDL_Renderer;

using namespace ROSE;

class Paddle : public Behavior {
public:
  Paddle() noexcept;
  static constexpr UUID typeID = "bceacc50f13cee94-7dbb93fec8659973"_uuid;
  static constexpr UUID TypeID() noexcept { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }

  static constexpr float width { 240.f };
  static constexpr float height { 24.f };
protected:
  void Unpack(const ParamView &view) override;
  void OnCreate() override;
  void OnStart() override;
  void FrameUpdate() override;
private:
  KeyCode m_keyLeft;
  KeyCode m_keyRight;
  enum Player {
    P1 = 1,
    P2 = 2
  } m_player;
  SDL_Renderer *m_renderer {};
  int m_screenW {};
  int m_screenH {};
};
