/**

  @file      input.cpp
  @brief     
  @details   ~
  @author    Viola Case
  @date      9.03.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>
#include <SDL3/SDL.h>

constexpr size_t KEYSTATE_SIZE = 512;

namespace ROSE {
  InputSystem::InputSystem() : keyState(SDL_GetKeyboardState(nullptr)), keyStatePrevious(new bool[KEYSTATE_SIZE]), gamepad(nullptr) {
    SDL_JoystickID *sticks = SDL_GetJoysticks(nullptr);
    if (!sticks) return;

    for (size_t i = 0; sticks[i] != 0; ++i) {
      auto j = sticks[i];
      if (SDL_IsGamepad(j)) gamepad = SDL_OpenGamepad(j);
    }

    SDL_free(sticks);
  }

  InputSystem::~InputSystem() {
    delete[] keyStatePrevious;
  }

  void InputSystem::Prime() {
    static InputSystem &inputSystem = GetInstance();
  }

  InputSystem &InputSystem::GetInstance() {
    static InputSystem inputSystem;
    return inputSystem;
  }

  bool InputSystem::GetKeyDown(KeyCode code) noexcept {
    static InputSystem &inputSystem = GetInstance();
    if (!inputSystem.keyState) return 0;
    return (inputSystem.keyState[code] && !inputSystem.keyStatePrevious[code]);
  }
  bool InputSystem::GetKeyUp(KeyCode code) noexcept {
    static InputSystem &inputSystem = GetInstance();
    if (!inputSystem.keyState) return 0;
    return !(inputSystem.keyState[code] && !inputSystem.keyStatePrevious[code]);
  }
  bool InputSystem::GetKey(KeyCode code) noexcept {
    static InputSystem &inputSystem = GetInstance();
    if (!inputSystem.keyState) return 0;
    return inputSystem.keyState[code];
  }

  void InputSystem::Init() noexcept {
    static InputSystem &inputSystem = GetInstance();
  }

  void InputSystem::Poll() noexcept {
    memcpy(keyStatePrevious, keyState, KEYSTATE_SIZE);
  }

  KeyCode::operator size_t() const noexcept { return value; }
  GamepadAxis::operator size_t() const noexcept { return value; }
  GamepadButton::operator size_t() const noexcept { return value; }

  float InputSystem::GetGamepadAxis(GamepadAxis axis) noexcept {
    static InputSystem &inputSystem = GetInstance();

    return (SDL_GetGamepadAxis(static_cast<SDL_Gamepad *>(inputSystem.gamepad), static_cast<SDL_GamepadAxis>(axis.value)) / 32767.f);
  }

  Vec2f InputSystem::GetStickAxes(GamepadStick stick) noexcept {
    static InputSystem &inputSystem = GetInstance();

    switch (static_cast<size_t>(stick)) {
    case static_cast<size_t>(GamepadStick::Left):
      return Vec2f(GetGamepadAxis(GamepadAxis::LeftStickX), GetGamepadAxis(GamepadAxis::LeftStickY));
    case static_cast<size_t>(GamepadStick::Right):
      return Vec2f(GetGamepadAxis(GamepadAxis::RightStickX), GetGamepadAxis(GamepadAxis::RightStickY));
    }
  }
  bool InputSystem::GetGamepadButton(GamepadButton button) noexcept {
    static InputSystem &inputSystem = GetInstance();
    return SDL_GetGamepadButton(static_cast<SDL_Gamepad *>(inputSystem.gamepad), static_cast<SDL_GamepadButton>(button.value));
  }

  String InputSystem::GetGamepadName() noexcept {
    static InputSystem &inputSystem = GetInstance();
    return SDL_GetGamepadName(static_cast<SDL_Gamepad *>(inputSystem.gamepad));
  }

#pragma region static KeyCodes

  const KeyCode KeyCode::A{ static_cast<int>(SDL_SCANCODE_A) };
  const KeyCode KeyCode::B{ static_cast<int>(SDL_SCANCODE_B) };
  const KeyCode KeyCode::C{ static_cast<int>(SDL_SCANCODE_C) };
  const KeyCode KeyCode::D{ static_cast<int>(SDL_SCANCODE_D) };
  const KeyCode KeyCode::E{ static_cast<int>(SDL_SCANCODE_E) };
  const KeyCode KeyCode::F{ static_cast<int>(SDL_SCANCODE_F) };
  const KeyCode KeyCode::G{ static_cast<int>(SDL_SCANCODE_G) };
  const KeyCode KeyCode::H{ static_cast<int>(SDL_SCANCODE_H) };
  const KeyCode KeyCode::I{ static_cast<int>(SDL_SCANCODE_I) };
  const KeyCode KeyCode::J{ static_cast<int>(SDL_SCANCODE_J) };
  const KeyCode KeyCode::K{ static_cast<int>(SDL_SCANCODE_K) };
  const KeyCode KeyCode::L{ static_cast<int>(SDL_SCANCODE_L) };
  const KeyCode KeyCode::M{ static_cast<int>(SDL_SCANCODE_M) };
  const KeyCode KeyCode::N{ static_cast<int>(SDL_SCANCODE_N) };
  const KeyCode KeyCode::O{ static_cast<int>(SDL_SCANCODE_O) };
  const KeyCode KeyCode::P{ static_cast<int>(SDL_SCANCODE_P) };
  const KeyCode KeyCode::Q{ static_cast<int>(SDL_SCANCODE_Q) };
  const KeyCode KeyCode::R{ static_cast<int>(SDL_SCANCODE_R) };
  const KeyCode KeyCode::S{ static_cast<int>(SDL_SCANCODE_S) };
  const KeyCode KeyCode::T{ static_cast<int>(SDL_SCANCODE_T) };
  const KeyCode KeyCode::U{ static_cast<int>(SDL_SCANCODE_U) };
  const KeyCode KeyCode::V{ static_cast<int>(SDL_SCANCODE_V) };
  const KeyCode KeyCode::W{ static_cast<int>(SDL_SCANCODE_W) };
  const KeyCode KeyCode::X{ static_cast<int>(SDL_SCANCODE_X) };
  const KeyCode KeyCode::Y{ static_cast<int>(SDL_SCANCODE_Y) };
  const KeyCode KeyCode::Z{ static_cast<int>(SDL_SCANCODE_Z) };
  const KeyCode KeyCode::ONE{ static_cast<int>(SDL_SCANCODE_1) };
  const KeyCode KeyCode::TWO{ static_cast<int>(SDL_SCANCODE_2) };
  const KeyCode KeyCode::THREE{ static_cast<int>(SDL_SCANCODE_3) };
  const KeyCode KeyCode::FOUR{ static_cast<int>(SDL_SCANCODE_4) };
  const KeyCode KeyCode::FIVE{ static_cast<int>(SDL_SCANCODE_5) };
  const KeyCode KeyCode::SIX{ static_cast<int>(SDL_SCANCODE_6) };
  const KeyCode KeyCode::SEVEN{ static_cast<int>(SDL_SCANCODE_7) };
  const KeyCode KeyCode::EIGHT{ static_cast<int>(SDL_SCANCODE_8) };
  const KeyCode KeyCode::NINE{ static_cast<int>(SDL_SCANCODE_9) };
  const KeyCode KeyCode::ZERO{ static_cast<int>(SDL_SCANCODE_0) };




#pragma endregion


  const GamepadButton GamepadButton::SOUTH{ SDL_GAMEPAD_BUTTON_SOUTH };
  const GamepadButton GamepadButton::EAST{ SDL_GAMEPAD_BUTTON_EAST };
  const GamepadButton GamepadButton::WEST{ SDL_GAMEPAD_BUTTON_WEST };
  const GamepadButton GamepadButton::NORTH{ SDL_GAMEPAD_BUTTON_NORTH };

  const GamepadButton GamepadButton::DPAD_UP{ SDL_GAMEPAD_BUTTON_DPAD_UP };
  const GamepadButton GamepadButton::DPAD_DOWN{ SDL_GAMEPAD_BUTTON_DPAD_DOWN };
  const GamepadButton GamepadButton::DPAD_LEFT{ SDL_GAMEPAD_BUTTON_DPAD_LEFT };
  const GamepadButton GamepadButton::DPAD_RIGHT{ SDL_GAMEPAD_BUTTON_DPAD_RIGHT };

  const GamepadButton GamepadButton::LEFT_STICK{ SDL_GAMEPAD_BUTTON_LEFT_STICK };
  const GamepadButton GamepadButton::RIGHT_STICK{ SDL_GAMEPAD_BUTTON_RIGHT_STICK };

  const GamepadButton GamepadButton::START{ SDL_GAMEPAD_BUTTON_START };
  const GamepadButton GamepadButton::BACK{ SDL_GAMEPAD_BUTTON_BACK };



  const GamepadAxis GamepadAxis::LeftStickX{ SDL_GAMEPAD_AXIS_LEFTX };
  const GamepadAxis GamepadAxis::LeftStickY{ SDL_GAMEPAD_AXIS_LEFTY };
  const GamepadAxis GamepadAxis::RightStickX{ SDL_GAMEPAD_AXIS_RIGHTX };
  const GamepadAxis GamepadAxis::RightStickY{ SDL_GAMEPAD_AXIS_RIGHTY };
  const GamepadAxis GamepadAxis::LeftTrigger{ SDL_GAMEPAD_AXIS_LEFT_TRIGGER };
  const GamepadAxis GamepadAxis::RightTrigger{ SDL_GAMEPAD_AXIS_RIGHT_TRIGGER };





































}