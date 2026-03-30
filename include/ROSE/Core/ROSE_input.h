/**

    @file      ROSE_input.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      3.03.2026
    @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_preamble.h>
#include <ROSE/Core/ROSE_math.h>

namespace ROSE {
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

#pragma region static Key Codes

    /**
    * @defgroup
    * @name Alphanumeric keys
    * @{
    */
    static const KeyCode A;                     //!< A
    static const KeyCode B;                     //!< B
    static const KeyCode C;                     //!< C
    static const KeyCode D;                     //!< D
    static const KeyCode E;                     //!< E
    static const KeyCode F;                     //!< F
    static const KeyCode G;                     //!< G
    static const KeyCode H;                     //!< H
    static const KeyCode I;                     //!< I
    static const KeyCode J;                     //!< J
    static const KeyCode K;                     //!< K
    static const KeyCode L;                     //!< L
    static const KeyCode M;                     //!< M
    static const KeyCode N;                     //!< N
    static const KeyCode O;                     //!< O
    static const KeyCode P;                     //!< P
    static const KeyCode Q;                     //!< Q
    static const KeyCode R;                     //!< R
    static const KeyCode S;                     //!< S
    static const KeyCode T;                     //!< T
    static const KeyCode U;                     //!< U
    static const KeyCode V;                     //!< V
    static const KeyCode W;                     //!< W
    static const KeyCode X;                     //!< X
    static const KeyCode Y;                     //!< Y
    static const KeyCode Z;                     //!< Z

    static const KeyCode ONE;                   //!< 1
    static const KeyCode TWO;                   //!< 2
    static const KeyCode THREE;                 //!< 3
    static const KeyCode FOUR;                  //!< 4
    static const KeyCode FIVE;                  //!< 5
    static const KeyCode SIX;                   //!< 6
    static const KeyCode SEVEN;                 //!< 7
    static const KeyCode EIGHT;                 //!< 8
    static const KeyCode NINE;                  //!< 9
    static const KeyCode ZERO;                  //!< 0
    /**
    * @}
    */

    /**
    * @defgroup
    * @name Punctuation keys
    * @{
    */
    static const KeyCode ENTER;                 //!< Enter
    static const KeyCode ESCAPE;                //!< Esc
    static const KeyCode BACKSPACE;             //!< Backspace
    static const KeyCode TAB;                   //!< Tab
    static const KeyCode SPACE;                 //!< Spacebar
    static const KeyCode MINUS;                 //!< -
    static const KeyCode EQUALS;                //!< =
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
    * @defgroup
    * @name Function keys
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
    * @defgroup
    * @name Navigation keys
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
    * @defgroup
    * @name Arrow keys
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
    * @defgroup
    * @name Keypad keys
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
    * @defgroup
    * @name Modifier keys
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
    * @defgroup
    * @name Media keys
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

  class GamepadAxis {
    unsigned int value;
    explicit constexpr GamepadAxis(unsigned int v) : value(v) {}
    friend class InputSystem;
    operator size_t() const noexcept;

  public:

#pragma region static Gamepad Axes

    static const GamepadAxis LeftStickX;
    static const GamepadAxis LeftStickY;
    static const GamepadAxis RightStickX;
    static const GamepadAxis RightStickY;
    static const GamepadAxis LeftTrigger;
    static const GamepadAxis RightTrigger;
#pragma endregion

  };

  class GamepadButton {
    unsigned int value;
    explicit constexpr GamepadButton(unsigned int v) : value(v) {}
    friend class InputSystem;
    operator size_t() const noexcept;
  public:
    static const GamepadButton SOUTH;
    static const GamepadButton EAST;
    static const GamepadButton WEST;
    static const GamepadButton NORTH;
    static const GamepadButton DPAD_UP;
    static const GamepadButton DPAD_DOWN;
    static const GamepadButton DPAD_LEFT;
    static const GamepadButton DPAD_RIGHT;
    static const GamepadButton LEFT_STICK;
    static const GamepadButton RIGHT_STICK;
    static const GamepadButton START;
    static const GamepadButton BACK;
    static const GamepadButton LEFT_BUMPER;
    static const GamepadButton RIGHT_BUMPER;
  };

  enum class GamepadStick {
    Left,
    Right
  };

  class InputSystem final {
  private:
    friend class Application;
    const bool *keyState;
    bool *keyStatePrevious;
    void *gamepad{ nullptr };
    InputSystem();
    ~InputSystem();
    void Init() noexcept;
    void Poll() noexcept;
    InputSystem(InputSystem &) = delete;
    InputSystem(InputSystem &&) = delete;
    static InputSystem inputSystem;
  public:
    static void Prime();
    static InputSystem &GetInstance();
    static bool GetKeyDown(KeyCode) noexcept;
    static bool GetKeyUp(KeyCode) noexcept;
    static bool GetKey(KeyCode) noexcept;

    static float GetGamepadAxis(GamepadAxis) noexcept;
    static Vec2f GetStickAxes(GamepadStick) noexcept;
    static bool GetGamepadButton(GamepadButton) noexcept;

    static String GetGamepadName() noexcept;

  };
}