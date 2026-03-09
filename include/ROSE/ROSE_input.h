/**

    @file      ROSE_input.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      3.03.2026
    @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/ROSE_preamble.h>
#include <SDL3/SDL_scancode.h>

namespace ROSE {
  struct KeyState {
    bool Pressed : 1;
    bool Down : 7;
  };
  /**
      @enum  ROSE::KeyCode
      @brief Think I'll sync it with SDL's scancodes or smth

  **/
  class KeyCode {
    unsigned int value;
    explicit constexpr KeyCode(unsigned int v) : value(v) {}
    friend class InputSystem;
    operator size_t() const noexcept;
  public:

#pragma region static KeyCodes

    /**
    * @defgroup Alphanumeric keys
    * @{
    */
    static const KeyCode A;
    static const KeyCode B;
    static const KeyCode C;
    static const KeyCode D;
    static const KeyCode E;
    static const KeyCode F;
    static const KeyCode G;
    static const KeyCode H;
    static const KeyCode I;
    static const KeyCode J;
    static const KeyCode K;
    static const KeyCode L;
    static const KeyCode M;
    static const KeyCode N;
    static const KeyCode O;
    static const KeyCode P;
    static const KeyCode Q;
    static const KeyCode R;
    static const KeyCode S;
    static const KeyCode T;
    static const KeyCode U;
    static const KeyCode V;
    static const KeyCode W;
    static const KeyCode X;
    static const KeyCode Y;
    static const KeyCode Z;

    static const KeyCode ONE;
    static const KeyCode TWO;
    static const KeyCode THREE;
    static const KeyCode FOUR;
    static const KeyCode FIVE;
    static const KeyCode SIX;
    static const KeyCode SEVEN;
    static const KeyCode EIGHT;
    static const KeyCode NINE;
    static const KeyCode ZERO;
    /**
    * @}
    */

    /**
    * @Punctuation keys
    * @{
    */
    static const KeyCode ENTER;
    static const KeyCode ESCAPE;
    static const KeyCode BACKSPACE;
    static const KeyCode TAB;
    static const KeyCode SPACE;
    static const KeyCode MINUS;
    static const KeyCode EQUALS;
    static const KeyCode LEFT_BRACKET;
    static const KeyCode RIGHT_BRACKET;
    static const KeyCode BACKSLASH;
    static const KeyCode APOSTROPHE;
    static const KeyCode GRAVE;
    static const KeyCode COMMA;
    static const KeyCode PERIOD;
    static const KeyCode SLASH;
    static const KeyCode CAPS_LOCK;
    /**
    * @}
    */

    /**
    * @defgroup Function keys
    * @{
    **/
    static const KeyCode F1;
    static const KeyCode F2;
    static const KeyCode F3;
    static const KeyCode F4;
    static const KeyCode F5;
    static const KeyCode F6;
    static const KeyCode F7;
    static const KeyCode F8;
    static const KeyCode F9;
    static const KeyCode F10;
    static const KeyCode F11;
    static const KeyCode F12;
    static const KeyCode F13;
    static const KeyCode F14;
    static const KeyCode F15;
    static const KeyCode F16;
    static const KeyCode F17;
    static const KeyCode F18;
    static const KeyCode F19;
    static const KeyCode F20;
    static const KeyCode F21;
    static const KeyCode F22;
    static const KeyCode F23;
    static const KeyCode F24;
    /**
    * @}
    **/

    static const KeyCode PRINTSCRN;
    static const KeyCode SCROLL_LOCK;
    static const KeyCode PAUSE;

    /**
    * @defgroup Navigation keys
    * @{
    **/
    static const KeyCode INSERT;
    static const KeyCode HOME;
    static const KeyCode END;
    static const KeyCode PGUP;
    static const KeyCode PGDN;
    static const KeyCode DELETE;
    /**
    * @}
    **/


    /**
    * @defgroup Arrow keys
    * @{
    **/
    static const KeyCode LEFT;
    static const KeyCode RIGHT;
    static const KeyCode UP;
    static const KeyCode DOWN;
    /**
    * @}
    **/
    /**
    * @defgroup Keypad keys
    * @{
    **/
    static const KeyCode NUM_LOCK;
    static const KeyCode KEYPAD_ONE;
    static const KeyCode KEYPAD_TWO;
    static const KeyCode KEYPAD_THREE;
    static const KeyCode KEYPAD_FOUR;
    static const KeyCode KEYPAD_FIVE;
    static const KeyCode KEYPAD_SIX;
    static const KeyCode KEYPAD_SEVEN;
    static const KeyCode KEYPAD_EIGHT;
    static const KeyCode KEYPAD_NINE;
    static const KeyCode KEYPAD_ZERO;
    static const KeyCode KEYPAD_PLUS;
    static const KeyCode KEYPAD_MINUS;
    static const KeyCode KEYPAD_MULTIPLY;
    static const KeyCode KEYPAD_DIVIDE;
    static const KeyCode KEYPAD_ENTER;
    static const KeyCode KEYPAD_PERIOD;
    static const KeyCode KEYPAD_EQUALS;
    /**
    * @}
    **/
    /**
    * @defgroup Modifier keys
    * @{
    **/
    static const KeyCode LEFT_CTRL;
    static const KeyCode LEFT_SHIFT;
    static const KeyCode LEFT_ALT;
    static const KeyCode RIGHT_CTRL;
    static const KeyCode RIGHT_SHIFT;
    static const KeyCode RIGHT_ALT;
    /**
    * @}
    **/

    /**
    * @defgroup Media keys
    * @{
    **/
    static const KeyCode MEDIA_PLAY;
    static const KeyCode MEDIA_PAUSE;
    static const KeyCode MEDIA_RECORD;
    static const KeyCode MEDIA_FAST_FORWARD;
    static const KeyCode MEDIA_REWIND;
    static const KeyCode MEDIA_NEXT;
    static const KeyCode MEDIA_PREVIOUS;
    static const KeyCode MEDIA_STOP;
    static const KeyCode MEDIA_EJECT;
    /**
    * @}
    **/

#pragma endregion

    constexpr bool operator==(const KeyCode &other) const { return value == other.value; }
    constexpr bool operator!=(const KeyCode &other) const { return value != other.value; }

  };

  class InputSystem final {
  private:
    friend class Application;
    const bool *keyState;
    bool *keyStatePrevious;
    InputSystem();
    ~InputSystem();
    void Poll() noexcept;
    InputSystem(InputSystem &) = delete;
    InputSystem(InputSystem &&) = delete;
  public:
    static InputSystem &get();
    static bool GetKeyDown(KeyCode) noexcept;
    static bool GetKeyUp(KeyCode) noexcept;
    static bool GetKey(KeyCode) noexcept;

  };
}