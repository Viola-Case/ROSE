/**

    @file      ROSE_input.h
    @brief     Input system providing keyboard and gamepad state queries
    @details   InputSystem is a singleton that mirrors the current-frame and
               previous-frame key/button states. Call Prime() (done automatically
               by Application::Init()) before querying any state. Poll() is
               driven by Application each frame.
    @author    Viola Case
    @date      3.03.2026
    @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <cstdlib>
#include <ROSE/Core/ROSE_rtl.h>
#include <ROSE/Core/ROSE_math.h>

namespace ROSE {
  /**
      @class   KeyCode
      @brief   Opaque keyboard scancode token.
      @details Compare against the static constants (KeyCode::A, KeyCode::SPACE,
               etc.) to identify keys. Values are mapped from SDL scancodes.

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

  /**
    @class   GamepadAxis
    @brief   Opaque token identifying one analog axis on a connected gamepad.
    @details Compare against the static constants (LeftStickX, RightTrigger, etc.)
             to select an axis, then read its value via InputSystem::GetGamepadAxis().
  **/
  class GamepadAxis {
    unsigned int value;
    explicit constexpr GamepadAxis(unsigned int v) : value(v) {}
    friend class InputSystem;
    operator size_t() const noexcept;

  public:

#pragma region static Gamepad Axes

    static const GamepadAxis LeftStickX;   //!< Left analog stick horizontal axis [-1, 1]
    static const GamepadAxis LeftStickY;   //!< Left analog stick vertical axis [-1, 1]
    static const GamepadAxis RightStickX;  //!< Right analog stick horizontal axis [-1, 1]
    static const GamepadAxis RightStickY;  //!< Right analog stick vertical axis [-1, 1]
    static const GamepadAxis LeftTrigger;  //!< Left trigger [0, 1]
    static const GamepadAxis RightTrigger; //!< Right trigger [0, 1]
#pragma endregion

  };

  /**
    @class   GamepadButton
    @brief   Opaque token identifying one digital button on a connected gamepad.
    @details Compare against the static constants and read button state via
             InputSystem::GetGamepadButton(). Button naming uses generic labels
             (SOUTH/EAST/WEST/NORTH) to remain controller-agnostic
             (e.g. SOUTH = A on Xbox, Cross on PlayStation).
  **/
  class GamepadButton {
    unsigned int value;
    explicit constexpr GamepadButton(unsigned int v) : value(v) {}
    friend class InputSystem;
    operator size_t() const noexcept;
  public:
    static const GamepadButton SOUTH;        //!< Bottom face button (A / Cross)
    static const GamepadButton EAST;         //!< Right face button (B / Circle)
    static const GamepadButton WEST;         //!< Left face button (X / Square)
    static const GamepadButton NORTH;        //!< Top face button (Y / Triangle)
    static const GamepadButton DPAD_UP;      //!< D-pad up
    static const GamepadButton DPAD_DOWN;    //!< D-pad down
    static const GamepadButton DPAD_LEFT;    //!< D-pad left
    static const GamepadButton DPAD_RIGHT;   //!< D-pad right
    static const GamepadButton LEFT_STICK;   //!< Left analog stick click (L3)
    static const GamepadButton RIGHT_STICK;  //!< Right analog stick click (R3)
    static const GamepadButton START;        //!< Start / Options button
    static const GamepadButton BACK;         //!< Back / Share / Select button
    static const GamepadButton LEFT_BUMPER;  //!< Left shoulder button (LB / L1)
    static const GamepadButton RIGHT_BUMPER; //!< Right shoulder button (RB / R1)
  };

  /**
    @enum  GamepadStick
    @brief Identifies which analog stick to read via InputSystem::GetStickAxes().
  **/
  enum class GamepadStick {
    Left,  //!< Left analog stick
    Right  //!< Right analog stick
  };

  /**
    @class   InputSystem
    @brief   Singleton that provides per-frame keyboard and gamepad state.
    @details All query methods are static and safe to call from any context
             after Prime() has been invoked. The system distinguishes between
             a key/button being pressed this frame only (GetKeyDown), released
             this frame only (GetKeyUp), and continuously held (GetKey).
  **/
  class InputSystem final {
  private:
    friend class Application;
    const bool *keyState;
    bool *keyStatePrevious;
    void *gamepad{ nullptr };
    InputSystem();
    ~InputSystem();

    /**
      @brief   Initialises SDL keyboard state pointers; called by Prime().
    **/
    void Init() noexcept;

    /**
      @brief   Copies current key state into the previous-frame buffer; called by Application each frame.
    **/
    void Poll() noexcept;
    InputSystem(InputSystem &) = delete;
    InputSystem(InputSystem &&) = delete;
    static InputSystem inputSystem;
  public:
    /**
      @brief   One-time initialisation; must be called before any query.
               Invoked automatically by Application::Init().
    **/
    static void Prime();

    /**
      @brief   Returns the singleton InputSystem instance.
    **/
    static InputSystem &GetInstance();

    /**
      @brief   Returns true on the first frame a key is pressed down.
      @param   Key code to query.
    **/
    static bool GetKeyDown(KeyCode) noexcept;

    /**
      @brief   Returns true on the first frame a key is released.
      @param   Key code to query.
    **/
    static bool GetKeyUp(KeyCode) noexcept;

    /**
      @brief   Returns true every frame the key is held down.
      @param   Key code to query.
    **/
    static bool GetKey(KeyCode) noexcept;

    /**
      @brief   Returns the normalized value of a single gamepad axis.
      @param   Axis to query.
      @retval  Value in [-1, 1] for sticks or [0, 1] for triggers; 0 if no gamepad is connected.
    **/
    static float GetGamepadAxis(GamepadAxis) noexcept;

    /**
      @brief   Returns both axes of an analog stick as a Vec2f.
      @param   Stick to query (Left or Right).
      @retval  {x, y} in [-1, 1]; {0, 0} if no gamepad is connected.
    **/
    static Vec2f GetStickAxes(GamepadStick) noexcept;

    /**
      @brief   Returns true every frame a gamepad button is held.
      @param   Button to query.
    **/
    static bool GetGamepadButton(GamepadButton) noexcept;

    /**
      @brief   Returns the product name of the first connected gamepad.
      @retval  Empty string if no gamepad is connected.
    **/
    static String GetGamepadName() noexcept;

  };
}
