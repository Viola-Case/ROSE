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
  InputSystem::InputSystem() : m_keyState(SDL_GetKeyboardState(nullptr)), m_keyStatePrevious(new bool[KEYSTATE_SIZE]), m_gamepad(nullptr) {
    SDL_JoystickID *sticks = SDL_GetJoysticks(nullptr);
    if (!sticks) return;

    for (size_t i = 0; sticks[i] != 0; ++i) {
      auto j = sticks[i];
      if (SDL_IsGamepad(j)) {
        m_gamepad = SDL_OpenGamepad(j); break;
      }
    }

    SDL_free(sticks);
  }

  InputSystem::~InputSystem() {
    delete[] m_keyStatePrevious;
  }

  void InputSystem::Prime() {
    static InputSystem &inputSystem = GetInstance();
  }

  InputSystem &InputSystem::GetInstance() {
    static InputSystem inputSystem;
    return inputSystem;
  }

  /*!
   * @return
   */
  bool InputSystem::GetKeyDown(KeyCode code) noexcept {
    static InputSystem &inputSystem = GetInstance();
    if (!inputSystem.m_keyState) return 0;
    return (inputSystem.m_keyState[code] && !inputSystem.m_keyStatePrevious[code]);
  }

  /*!
   * @return
   */
  bool InputSystem::GetKeyUp(KeyCode code) noexcept {
    static InputSystem &inputSystem = GetInstance();
    if (!inputSystem.m_keyState) return 0;
    return (!inputSystem.m_keyState[code] && inputSystem.m_keyStatePrevious[code]);
  }

  /*!
   * @return State of key in current frame
   */
  bool InputSystem::GetKey(KeyCode code) noexcept {
    static InputSystem &inputSystem = GetInstance();
    if (!inputSystem.m_keyState) return 0;
    return inputSystem.m_keyState[code];
  }

  /*!
   * @note This pointer is constant from life until death. Recommended to call once per object at start.
   * @return Pointer to keyboard state.
   */
  const bool *InputSystem::GetKeyboardPtr() const noexcept {
    return m_keyState;
  }

  /*!
   * @note It's unlikely you'd use this, but I'm leaving it available just in case.
   * @return
   */
  const bool *InputSystem::GetPreviousKeyboardPtr() const noexcept {
    return m_keyStatePrevious;
  }

  void InputSystem::Init() noexcept {
    static InputSystem &inputSystem = GetInstance();
  }

  void InputSystem::Poll() noexcept {
    memcpy(m_keyStatePrevious, m_keyState, KEYSTATE_SIZE);
  }

  KeyCode::operator size_t() const noexcept { return value; }
  GamepadAxis::operator size_t() const noexcept { return value; }
  GamepadButton::operator size_t() const noexcept { return value; }

  float InputSystem::GetGamepadAxis(GamepadAxis axis) noexcept {
    static InputSystem &inputSystem = GetInstance();

    return (SDL_GetGamepadAxis(static_cast<SDL_Gamepad *>(inputSystem.m_gamepad), static_cast<SDL_GamepadAxis>(axis.value)) / 32767.f);
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
    return SDL_GetGamepadButton(static_cast<SDL_Gamepad *>(GetInstance().m_gamepad), static_cast<SDL_GamepadButton>(button.value));
  }

  String InputSystem::GetGamepadName() noexcept {
    static InputSystem &inputSystem = GetInstance();
    return String(SDL_GetGamepadName(static_cast<SDL_Gamepad *>(inputSystem.m_gamepad)));
  }

#pragma region static KeyCodes

  constinit const KeyCode KeyCode::A{ static_cast<int>(SDL_SCANCODE_A) };
  constinit const KeyCode KeyCode::B{ static_cast<int>(SDL_SCANCODE_B) };
  constinit const KeyCode KeyCode::C{ static_cast<int>(SDL_SCANCODE_C) };
  constinit const KeyCode KeyCode::D{ static_cast<int>(SDL_SCANCODE_D) };
  constinit const KeyCode KeyCode::E{ static_cast<int>(SDL_SCANCODE_E) };
  constinit const KeyCode KeyCode::F{ static_cast<int>(SDL_SCANCODE_F) };
  constinit const KeyCode KeyCode::G{ static_cast<int>(SDL_SCANCODE_G) };
  constinit const KeyCode KeyCode::H{ static_cast<int>(SDL_SCANCODE_H) };
  constinit const KeyCode KeyCode::I{ static_cast<int>(SDL_SCANCODE_I) };
  constinit const KeyCode KeyCode::J{ static_cast<int>(SDL_SCANCODE_J) };
  constinit const KeyCode KeyCode::K{ static_cast<int>(SDL_SCANCODE_K) };
  constinit const KeyCode KeyCode::L{ static_cast<int>(SDL_SCANCODE_L) };
  constinit const KeyCode KeyCode::M{ static_cast<int>(SDL_SCANCODE_M) };
  constinit const KeyCode KeyCode::N{ static_cast<int>(SDL_SCANCODE_N) };
  constinit const KeyCode KeyCode::O{ static_cast<int>(SDL_SCANCODE_O) };
  constinit const KeyCode KeyCode::P{ static_cast<int>(SDL_SCANCODE_P) };
  constinit const KeyCode KeyCode::Q{ static_cast<int>(SDL_SCANCODE_Q) };
  constinit const KeyCode KeyCode::R{ static_cast<int>(SDL_SCANCODE_R) };
  constinit const KeyCode KeyCode::S{ static_cast<int>(SDL_SCANCODE_S) };
  constinit const KeyCode KeyCode::T{ static_cast<int>(SDL_SCANCODE_T) };
  constinit const KeyCode KeyCode::U{ static_cast<int>(SDL_SCANCODE_U) };
  constinit const KeyCode KeyCode::V{ static_cast<int>(SDL_SCANCODE_V) };
  constinit const KeyCode KeyCode::W{ static_cast<int>(SDL_SCANCODE_W) };
  constinit const KeyCode KeyCode::X{ static_cast<int>(SDL_SCANCODE_X) };
  constinit const KeyCode KeyCode::Y{ static_cast<int>(SDL_SCANCODE_Y) };
  constinit const KeyCode KeyCode::Z{ static_cast<int>(SDL_SCANCODE_Z) };
  constinit const KeyCode KeyCode::ONE{ static_cast<int>(SDL_SCANCODE_1) };
  constinit const KeyCode KeyCode::TWO{ static_cast<int>(SDL_SCANCODE_2) };
  constinit const KeyCode KeyCode::THREE{ static_cast<int>(SDL_SCANCODE_3) };
  constinit const KeyCode KeyCode::FOUR{ static_cast<int>(SDL_SCANCODE_4) };
  constinit const KeyCode KeyCode::FIVE{ static_cast<int>(SDL_SCANCODE_5) };
  constinit const KeyCode KeyCode::SIX{ static_cast<int>(SDL_SCANCODE_6) };
  constinit const KeyCode KeyCode::SEVEN{ static_cast<int>(SDL_SCANCODE_7) };
  constinit const KeyCode KeyCode::EIGHT{ static_cast<int>(SDL_SCANCODE_8) };
  constinit const KeyCode KeyCode::NINE{ static_cast<int>(SDL_SCANCODE_9) };
  constinit const KeyCode KeyCode::ZERO{ static_cast<int>(SDL_SCANCODE_0) };




#pragma endregion


  constinit const GamepadButton GamepadButton::SOUTH{ SDL_GAMEPAD_BUTTON_SOUTH };
  constinit const GamepadButton GamepadButton::EAST{ SDL_GAMEPAD_BUTTON_EAST };
  constinit const GamepadButton GamepadButton::WEST{ SDL_GAMEPAD_BUTTON_WEST };
  constinit const GamepadButton GamepadButton::NORTH{ SDL_GAMEPAD_BUTTON_NORTH };

  constinit const GamepadButton GamepadButton::DPAD_UP{ SDL_GAMEPAD_BUTTON_DPAD_UP };
  constinit const GamepadButton GamepadButton::DPAD_DOWN{ SDL_GAMEPAD_BUTTON_DPAD_DOWN };
  constinit const GamepadButton GamepadButton::DPAD_LEFT{ SDL_GAMEPAD_BUTTON_DPAD_LEFT };
  constinit const GamepadButton GamepadButton::DPAD_RIGHT{ SDL_GAMEPAD_BUTTON_DPAD_RIGHT };

  constinit const GamepadButton GamepadButton::LEFT_STICK{ SDL_GAMEPAD_BUTTON_LEFT_STICK };
  constinit const GamepadButton GamepadButton::RIGHT_STICK{ SDL_GAMEPAD_BUTTON_RIGHT_STICK };

  constinit const GamepadButton GamepadButton::START{ SDL_GAMEPAD_BUTTON_START };
  constinit const GamepadButton GamepadButton::BACK{ SDL_GAMEPAD_BUTTON_BACK };



  constinit const GamepadAxis GamepadAxis::LeftStickX{ SDL_GAMEPAD_AXIS_LEFTX };
  constinit const GamepadAxis GamepadAxis::LeftStickY{ SDL_GAMEPAD_AXIS_LEFTY };
  constinit const GamepadAxis GamepadAxis::RightStickX{ SDL_GAMEPAD_AXIS_RIGHTX };
  constinit const GamepadAxis GamepadAxis::RightStickY{ SDL_GAMEPAD_AXIS_RIGHTY };
  constinit const GamepadAxis GamepadAxis::LeftTrigger{ SDL_GAMEPAD_AXIS_LEFT_TRIGGER };
  constinit const GamepadAxis GamepadAxis::RightTrigger{ SDL_GAMEPAD_AXIS_RIGHT_TRIGGER };





































}