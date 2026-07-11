/**

    @file      input.h
    @brief
    @details   ~
    @author    Viola Case
    @date      3.03.2026
    @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <cstdlib>
#include <ROSE/Core/rtl.h>
#include <ROSE/Core/math.h>

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

    char ToChar() const noexcept;
#pragma region static Key Codes

    static const KeyCode Unknown;

    /**
     * @defgroup
     * @name Alphanumeric keys
     * @{
     */
    static const KeyCode A; //!< A
    static const KeyCode B; //!< B
    static const KeyCode C; //!< C
    static const KeyCode D; //!< D
    static const KeyCode E; //!< E
    static const KeyCode F; //!< F
    static const KeyCode G; //!< G
    static const KeyCode H; //!< H
    static const KeyCode I; //!< I
    static const KeyCode J; //!< J
    static const KeyCode K; //!< K
    static const KeyCode L; //!< L
    static const KeyCode M; //!< M
    static const KeyCode N; //!< N
    static const KeyCode O; //!< O
    static const KeyCode P; //!< P
    static const KeyCode Q; //!< Q
    static const KeyCode R; //!< R
    static const KeyCode S; //!< S
    static const KeyCode T; //!< T
    static const KeyCode U; //!< U
    static const KeyCode V; //!< V
    static const KeyCode W; //!< W
    static const KeyCode X; //!< X
    static const KeyCode Y; //!< Y
    static const KeyCode Z; //!< Z

    static const KeyCode ONE;   //!< 1
    static const KeyCode TWO;   //!< 2
    static const KeyCode THREE; //!< 3
    static const KeyCode FOUR;  //!< 4
    static const KeyCode FIVE;  //!< 5
    static const KeyCode SIX;   //!< 6
    static const KeyCode SEVEN; //!< 7
    static const KeyCode EIGHT; //!< 8
    static const KeyCode NINE;  //!< 9
    static const KeyCode ZERO;  //!< 0
    /**
     * @}
     */

    /**
     * @defgroup
     * @name Punctuation keys
     * @{
     */
    static const KeyCode ENTER;     //!< Enter
    static const KeyCode ESCAPE;    //!< Esc
    static const KeyCode BACKSPACE; //!< Backspace
    static const KeyCode TAB;       //!< Tab
    static const KeyCode SPACE;     //!< Spacebar
    static const KeyCode MINUS;     //!< `-`
    static const KeyCode EQUALS;    //!< `=`
    static const KeyCode LEFT_BRACKET; //!< `[`
    static const KeyCode RIGHT_BRACKET; //!< `]`
    static const KeyCode BACKSLASH; //!< `\`
    static const KeyCode SEMICOLON; //!< `;`
    static const KeyCode APOSTROPHE; //!< `'`
    static const KeyCode GRAVE; //!< ```
    static const KeyCode COMMA; //!< `,`
    static const KeyCode PERIOD; //!< `.`
    static const KeyCode SLASH; //!< `/`
    static const KeyCode CAPS_LOCK; //!< Caps lock
    /**
     * @}
     */

    /**
     * @defgroup
     * @name Function keys
     * @{
     **/
    static const KeyCode F1; //!< F1
    static const KeyCode F2; //!< F2
    static const KeyCode F3; //!< F3
    static const KeyCode F4; //!< F4
    static const KeyCode F5; //!< F5
    static const KeyCode F6; //!< F6
    static const KeyCode F7; //!< F7
    static const KeyCode F8; //!< F8
    static const KeyCode F9; //!< F9
    static const KeyCode F10; //!< F10
    static const KeyCode F11; //!< F11
    static const KeyCode F12; //!< F12
    static const KeyCode F13; //!< F13
    static const KeyCode F14; //!< F14
    static const KeyCode F15; //!< F15
    static const KeyCode F16; //!< F16
    static const KeyCode F17; //!< F17
    static const KeyCode F18; //!< F18
    static const KeyCode F19; //!< F19
    static const KeyCode F20; //!< F20
    static const KeyCode F21; //!< F21
    static const KeyCode F22; //!< F22
    static const KeyCode F23; //!< F23
    static const KeyCode F24; //!< F24
    /**
     * @}
     **/

    /**
     * @defgroup
     * @name Navigation keys
     * @{
     **/
    static const KeyCode INSERT;
    static const KeyCode HOME;
    static const KeyCode END;
    static const KeyCode PAGE_UP;
    static const KeyCode PAGE_DOWN;
    static const KeyCode DELETE;
    /**
     * @}
     **/


    /**
     * @defgroup
     * @name Arrow keys
     * @{
     **/
    static const KeyCode LEFT; //!< ←
    static const KeyCode RIGHT; //!< →
    static const KeyCode UP; //!< ↑
    static const KeyCode DOWN; //!< ↓
    /**
     * @}
     **/
    /**
     * @defgroup
     * @name Keypad keys
     * @{
     **/
    static const KeyCode NUM_LOCK; //!< Num lock
    static const KeyCode KEYPAD_ONE; //!< (On the keypad)
    static const KeyCode KEYPAD_TWO; //!< (On the keypad)
    static const KeyCode KEYPAD_THREE; //!< (On the keypad)
    static const KeyCode KEYPAD_FOUR; //!< (On the keypad)
    static const KeyCode KEYPAD_FIVE; //!< (On the keypad)
    static const KeyCode KEYPAD_SIX; //!< (On the keypad)
    static const KeyCode KEYPAD_SEVEN; //!< (On the keypad)
    static const KeyCode KEYPAD_EIGHT; //!< (On the keypad)
    static const KeyCode KEYPAD_NINE; //!< (On the keypad)
    static const KeyCode KEYPAD_ZERO; //!< (On the keypad)
    static const KeyCode KEYPAD_PLUS; //!< (On the keypad)
    static const KeyCode KEYPAD_MINUS; //!< (On the keypad)
    static const KeyCode KEYPAD_MULTIPLY; //!< (On the keypad)
    static const KeyCode KEYPAD_DIVIDE; //!< (On the keypad)
    static const KeyCode KEYPAD_ENTER; //!< (On the keypad)
    static const KeyCode KEYPAD_PERIOD; //!< (On the keypad)
    static const KeyCode KEYPAD_EQUALS; //!< (On the keypad)
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

  class JoystickAxis {

  };

  class InputSystem final {
    friend class Application;
    const bool *m_keyState;
    bool *m_keyStatePrevious;
    void *m_gamepad { nullptr };
    InputSystem();
    ~InputSystem();
    void Init() noexcept;
    void Poll() noexcept;
    InputSystem(InputSystem &) = delete;
    InputSystem(InputSystem &&) = delete;
    static InputSystem inputSystem;

  public:
    /**
     * This doesn't do much on its own but
     */
    static void Prime();
    static InputSystem &GetInstance();
    static bool GetKeyDown(KeyCode) noexcept;
    static bool GetKeyUp(KeyCode) noexcept;
    static bool GetKey(KeyCode) noexcept;

    static float GetGamepadAxis(GamepadAxis) noexcept;
    static Vec2f GetStickAxes(GamepadStick) noexcept;
    static bool GetGamepadButton(GamepadButton) noexcept;


    const bool *GetKeyboardPtr() const noexcept;

    const bool *GetPreviousKeyboardPtr() const noexcept;

    static String GetGamepadName() noexcept;
  };


} // namespace ROSE