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
  InputSystem::InputSystem() : keyState(SDL_GetKeyboardState(nullptr)), keyStatePrevious(new bool[KEYSTATE_SIZE]) {}

  InputSystem::~InputSystem() {
    delete[] keyStatePrevious;
  }

  InputSystem &InputSystem::GetInstance() {
    static InputSystem &inputSystem = GetInstance();
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













































}