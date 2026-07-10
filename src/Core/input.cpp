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

  /**
   *
   * @param stick stick
   * @return Vector from (-1,-1) to (1,1) representing the corresponding stick. Will return zero vector if given an unknown stick.
   */
  Vec2f InputSystem::GetStickAxes(GamepadStick stick) noexcept {
    static InputSystem &inputSystem = GetInstance();

    switch (static_cast<size_t>(stick)) {
    case static_cast<size_t>(GamepadStick::Left):
      return Vec2f(GetGamepadAxis(GamepadAxis::LeftStickX), GetGamepadAxis(GamepadAxis::LeftStickY));
    case static_cast<size_t>(GamepadStick::Right):
      return Vec2f(GetGamepadAxis(GamepadAxis::RightStickX), GetGamepadAxis(GamepadAxis::RightStickY));
    default:
      return Vec2f{};
    }
  }

  bool InputSystem::GetGamepadButton(GamepadButton button) noexcept {
    return SDL_GetGamepadButton(static_cast<SDL_Gamepad *>(GetInstance().m_gamepad), static_cast<SDL_GamepadButton>(button.value));
  }

  String InputSystem::GetGamepadName() noexcept {
    static InputSystem &inputSystem = GetInstance();
    return String(SDL_GetGamepadName(static_cast<SDL_Gamepad *>(inputSystem.m_gamepad)));
  }

  /*!
   * @return The character this key produces with no modifiers held, under the current
   *         keyboard layout. '\0' if the key has no printable character.
   */
  char KeyCode::ToChar() const noexcept {
    // Keypad keys have dedicated keycodes with no printable value, so map their glyphs by hand.
    if (SDL_SCANCODE_KP_1 <= value && value <= SDL_SCANCODE_KP_9)
      return static_cast<char>(value - SDL_SCANCODE_KP_1 + '1');
    switch (value) {
    case SDL_SCANCODE_KP_0:        return '0';
    case SDL_SCANCODE_KP_PERIOD:   return '.';
    case SDL_SCANCODE_KP_PLUS:     return '+';
    case SDL_SCANCODE_KP_MINUS:    return '-';
    case SDL_SCANCODE_KP_MULTIPLY: return '*';
    case SDL_SCANCODE_KP_DIVIDE:   return '/';
    case SDL_SCANCODE_KP_EQUALS:   return '=';
    default:                       break;
    }

    // For everything else, SDL knows what character this physical key produces
    // (falls back to QWERTY when no layout is available yet).
    SDL_Keycode key = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(value), SDL_KMOD_NONE, false);
    return (' ' <= key && key <= '~') ? static_cast<char>(key) : '\0';
  }

#pragma region static KeyCodes

  constinit const KeyCode KeyCode::Unknown{ static_cast<int>(SDL_SCANCODE_UNKNOWN) };

  constinit const KeyCode KeyCode::A{ SDL_SCANCODE_A };
  constinit const KeyCode KeyCode::B{ SDL_SCANCODE_B };
  constinit const KeyCode KeyCode::C{ SDL_SCANCODE_C };
  constinit const KeyCode KeyCode::D{ SDL_SCANCODE_D };
  constinit const KeyCode KeyCode::E{ SDL_SCANCODE_E };
  constinit const KeyCode KeyCode::F{ SDL_SCANCODE_F };
  constinit const KeyCode KeyCode::G{ SDL_SCANCODE_G };
  constinit const KeyCode KeyCode::H{ SDL_SCANCODE_H };
  constinit const KeyCode KeyCode::I{ SDL_SCANCODE_I };
  constinit const KeyCode KeyCode::J{ SDL_SCANCODE_J };
  constinit const KeyCode KeyCode::K{ SDL_SCANCODE_K };
  constinit const KeyCode KeyCode::L{ SDL_SCANCODE_L };
  constinit const KeyCode KeyCode::M{ SDL_SCANCODE_M };
  constinit const KeyCode KeyCode::N{ SDL_SCANCODE_N };
  constinit const KeyCode KeyCode::O{ SDL_SCANCODE_O };
  constinit const KeyCode KeyCode::P{ SDL_SCANCODE_P };
  constinit const KeyCode KeyCode::Q{ SDL_SCANCODE_Q };
  constinit const KeyCode KeyCode::R{ SDL_SCANCODE_R };
  constinit const KeyCode KeyCode::S{ SDL_SCANCODE_S };
  constinit const KeyCode KeyCode::T{ SDL_SCANCODE_T };
  constinit const KeyCode KeyCode::U{ SDL_SCANCODE_U };
  constinit const KeyCode KeyCode::V{ SDL_SCANCODE_V };
  constinit const KeyCode KeyCode::W{ SDL_SCANCODE_W };
  constinit const KeyCode KeyCode::X{ SDL_SCANCODE_X };
  constinit const KeyCode KeyCode::Y{ SDL_SCANCODE_Y };
  constinit const KeyCode KeyCode::Z{ SDL_SCANCODE_Z };

  constinit const KeyCode KeyCode::ONE{ SDL_SCANCODE_1 };
  constinit const KeyCode KeyCode::TWO{ SDL_SCANCODE_2 };
  constinit const KeyCode KeyCode::THREE{ SDL_SCANCODE_3 };
  constinit const KeyCode KeyCode::FOUR{ SDL_SCANCODE_4 };
  constinit const KeyCode KeyCode::FIVE{ SDL_SCANCODE_5 };
  constinit const KeyCode KeyCode::SIX{ SDL_SCANCODE_6 };
  constinit const KeyCode KeyCode::SEVEN{ SDL_SCANCODE_7 };
  constinit const KeyCode KeyCode::EIGHT{ SDL_SCANCODE_8 };
  constinit const KeyCode KeyCode::NINE{ SDL_SCANCODE_9 };
  constinit const KeyCode KeyCode::ZERO{ SDL_SCANCODE_0 };

  constinit const KeyCode KeyCode::RIGHT{ SDL_SCANCODE_RIGHT };
  constinit const KeyCode KeyCode::LEFT{ SDL_SCANCODE_LEFT };
  constinit const KeyCode KeyCode::DOWN{ SDL_SCANCODE_DOWN };
  constinit const KeyCode KeyCode::UP{ SDL_SCANCODE_UP };

  constinit const KeyCode KeyCode::F1{ SDL_SCANCODE_F1 };
  constinit const KeyCode KeyCode::F2{ SDL_SCANCODE_F2 };
  constinit const KeyCode KeyCode::F3{ SDL_SCANCODE_F3 };
  constinit const KeyCode KeyCode::F4{ SDL_SCANCODE_F4 };
  constinit const KeyCode KeyCode::F5{ SDL_SCANCODE_F5 };
  constinit const KeyCode KeyCode::F6{ SDL_SCANCODE_F6 };
  constinit const KeyCode KeyCode::F7{ SDL_SCANCODE_F7 };
  constinit const KeyCode KeyCode::F8{ SDL_SCANCODE_F8 };
  constinit const KeyCode KeyCode::F9{ SDL_SCANCODE_F9 };
  constinit const KeyCode KeyCode::F10{ SDL_SCANCODE_F10 };
  constinit const KeyCode KeyCode::F11{ SDL_SCANCODE_F11 };
  constinit const KeyCode KeyCode::F12{ SDL_SCANCODE_F12 };
  constinit const KeyCode KeyCode::F13{ SDL_SCANCODE_F13 };
  constinit const KeyCode KeyCode::F14{ SDL_SCANCODE_F14 };
  constinit const KeyCode KeyCode::F15{ SDL_SCANCODE_F15 };
  constinit const KeyCode KeyCode::F16{ SDL_SCANCODE_F16 };
  constinit const KeyCode KeyCode::F17{ SDL_SCANCODE_F17 };
  constinit const KeyCode KeyCode::F18{ SDL_SCANCODE_F18 };
  constinit const KeyCode KeyCode::F19{ SDL_SCANCODE_F19 };
  constinit const KeyCode KeyCode::F20{ SDL_SCANCODE_F20 };
  constinit const KeyCode KeyCode::F21{ SDL_SCANCODE_F21 };
  constinit const KeyCode KeyCode::F22{ SDL_SCANCODE_F22 };
  constinit const KeyCode KeyCode::F23{ SDL_SCANCODE_F23 };
  constinit const KeyCode KeyCode::F24{ SDL_SCANCODE_F24 };

  constinit const KeyCode KeyCode::NUM_LOCK{ SDL_SCANCODE_NUMLOCKCLEAR };
  constinit const KeyCode KeyCode::KEYPAD_ONE{ SDL_SCANCODE_KP_1 };
  constinit const KeyCode KeyCode::KEYPAD_TWO{ SDL_SCANCODE_KP_2 };
  constinit const KeyCode KeyCode::KEYPAD_THREE{ SDL_SCANCODE_KP_3 };
  constinit const KeyCode KeyCode::KEYPAD_FOUR{ SDL_SCANCODE_KP_4 };
  constinit const KeyCode KeyCode::KEYPAD_FIVE{ SDL_SCANCODE_KP_5 };
  constinit const KeyCode KeyCode::KEYPAD_SIX{ SDL_SCANCODE_KP_6 };
  constinit const KeyCode KeyCode::KEYPAD_SEVEN{ SDL_SCANCODE_KP_7 };
  constinit const KeyCode KeyCode::KEYPAD_EIGHT{ SDL_SCANCODE_KP_8 };
  constinit const KeyCode KeyCode::KEYPAD_NINE{ SDL_SCANCODE_KP_9 };
  constinit const KeyCode KeyCode::KEYPAD_ZERO{ SDL_SCANCODE_KP_0 };
  constinit const KeyCode KeyCode::KEYPAD_PLUS{ SDL_SCANCODE_KP_PLUS };
  constinit const KeyCode KeyCode::KEYPAD_MINUS{ SDL_SCANCODE_KP_MINUS };
  constinit const KeyCode KeyCode::KEYPAD_MULTIPLY{ SDL_SCANCODE_KP_MULTIPLY };
  constinit const KeyCode KeyCode::KEYPAD_DIVIDE{ SDL_SCANCODE_KP_DIVIDE };
  constinit const KeyCode KeyCode::KEYPAD_ENTER{ SDL_SCANCODE_KP_ENTER };
  constinit const KeyCode KeyCode::KEYPAD_PERIOD{ SDL_SCANCODE_KP_PERIOD };
  constinit const KeyCode KeyCode::KEYPAD_EQUALS{ SDL_SCANCODE_KP_EQUALS };

  constinit const KeyCode KeyCode::LEFT_CTRL{ SDL_SCANCODE_LCTRL };
  constinit const KeyCode KeyCode::LEFT_SHIFT{ SDL_SCANCODE_LSHIFT };
  constinit const KeyCode KeyCode::LEFT_ALT{ SDL_SCANCODE_LALT };
  constinit const KeyCode KeyCode::RIGHT_CTRL{ SDL_SCANCODE_RCTRL };
  constinit const KeyCode KeyCode::RIGHT_SHIFT{ SDL_SCANCODE_RSHIFT };
  constinit const KeyCode KeyCode::RIGHT_ALT{ SDL_SCANCODE_RALT };

  constinit const KeyCode KeyCode::INSERT{ SDL_SCANCODE_INSERT };
  constinit const KeyCode KeyCode::HOME{ SDL_SCANCODE_HOME };
  constinit const KeyCode KeyCode::END{ SDL_SCANCODE_END };
  constinit const KeyCode KeyCode::PAGE_UP{ SDL_SCANCODE_PAGEUP };
  constinit const KeyCode KeyCode::PAGE_DOWN{ SDL_SCANCODE_PAGEDOWN };
  constinit const KeyCode KeyCode::DELETE{ SDL_SCANCODE_DELETE };


  constinit const KeyCode KeyCode::ENTER{ SDL_SCANCODE_RETURN };
  constinit const KeyCode KeyCode::ESCAPE{ SDL_SCANCODE_ESCAPE };
  constinit const KeyCode KeyCode::BACKSPACE{ SDL_SCANCODE_BACKSPACE };
  constinit const KeyCode KeyCode::TAB{ SDL_SCANCODE_TAB };
  constinit const KeyCode KeyCode::SPACE{ SDL_SCANCODE_SPACE };
  constinit const KeyCode KeyCode::MINUS{ SDL_SCANCODE_MINUS };
  constinit const KeyCode KeyCode::EQUALS{ SDL_SCANCODE_EQUALS };
  constinit const KeyCode KeyCode::LEFT_BRACKET{ SDL_SCANCODE_LEFTBRACKET };
  constinit const KeyCode KeyCode::RIGHT_BRACKET{ SDL_SCANCODE_RIGHTBRACKET };
  constinit const KeyCode KeyCode::BACKSLASH{ SDL_SCANCODE_BACKSLASH };
  constinit const KeyCode KeyCode::SEMICOLON{ SDL_SCANCODE_SEMICOLON };
  constinit const KeyCode KeyCode::APOSTROPHE{ SDL_SCANCODE_APOSTROPHE };
  constinit const KeyCode KeyCode::GRAVE{ SDL_SCANCODE_GRAVE };
  constinit const KeyCode KeyCode::COMMA{ SDL_SCANCODE_COMMA };
  constinit const KeyCode KeyCode::PERIOD{ SDL_SCANCODE_PERIOD };
  constinit const KeyCode KeyCode::SLASH{ SDL_SCANCODE_SLASH };
  constinit const KeyCode KeyCode::CAPS_LOCK{ SDL_SCANCODE_CAPSLOCK };






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

  constinit const GamepadButton GamepadButton::LEFT_BUMPER{ SDL_GAMEPAD_BUTTON_LEFT_SHOULDER };
  constinit const GamepadButton GamepadButton::RIGHT_BUMPER{ SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER };



  constinit const GamepadAxis GamepadAxis::LeftStickX{ SDL_GAMEPAD_AXIS_LEFTX };
  constinit const GamepadAxis GamepadAxis::LeftStickY{ SDL_GAMEPAD_AXIS_LEFTY };
  constinit const GamepadAxis GamepadAxis::RightStickX{ SDL_GAMEPAD_AXIS_RIGHTX };
  constinit const GamepadAxis GamepadAxis::RightStickY{ SDL_GAMEPAD_AXIS_RIGHTY };
  constinit const GamepadAxis GamepadAxis::LeftTrigger{ SDL_GAMEPAD_AXIS_LEFT_TRIGGER };
  constinit const GamepadAxis GamepadAxis::RightTrigger{ SDL_GAMEPAD_AXIS_RIGHT_TRIGGER };





































}