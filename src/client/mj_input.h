// Input library - Marco Jonkers 2020
// Should not use pre-compiled headers (for portability)
// Only supports SDL for now

#pragma once
#include <stdint.h>

#ifdef MJ_INPUT_SDL
#include <SDL_keycode.h>
#endif // MJ_INPUT_SDL

struct GamepadHandle
{
  uint16_t idx;
};

inline bool isValid(GamepadHandle _handle)
{
  return UINT16_MAX != _handle.idx;
}

struct MouseButton
{
  enum Enum
  {
    None,
    Left,
    Right,
    Middle,
    Back,
    Forward,
    Count
  };
};

struct GamepadAxis
{
  enum Enum
  {
    LeftX,
    LeftY,
    LeftZ,
    RightX,
    RightY,
    RightZ,

    Count
  };
};

// Only used for initialization of controls
struct Modifier
{
  enum Enum
  {
    None       = 0,
    LeftAlt    = 0x01,
    RightAlt   = 0x02,
    LeftCtrl   = 0x04,
    RightCtrl  = 0x08,
    LeftShift  = 0x10,
    RightShift = 0x20,
    LeftMeta   = 0x40,
    RightMeta  = 0x80,
  };
};

union ModifierMask {
  uint8_t all;
  struct
  {
    uint8_t LeftAlt : 1;
    uint8_t RightAlt : 1;
    uint8_t LeftCtrl : 1;
    uint8_t RightCtrl : 1;
    uint8_t LeftShift : 1;
    uint8_t RightShift : 1;
    uint8_t LeftMeta : 1;
    uint8_t RightMeta : 1;
  };
};

/**
 * @brief      Virtual keys. Platform maps physical keys to these.
 */
struct Key
{
  enum Enum
  {
    None = 0,
    Esc,
    Return,
    Tab,
    Space,
    Backspace,
    Up,
    Down,
    Left,
    Right,
    Insert,
    Delete,
    Home,
    End,
    PageUp,
    PageDown,
    Print,
    Equals,
    Minus,
    LeftBracket,
    RightBracket,
    Semicolon,
    Quote,
    Comma,
    Period,
    Slash,
    Backslash,
    Tilde,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    NumPad0,
    NumPad1,
    NumPad2,
    NumPad3,
    NumPad4,
    NumPad5,
    NumPad6,
    NumPad7,
    NumPad8,
    NumPad9,
    Key0,
    Key1,
    Key2,
    Key3,
    Key4,
    Key5,
    Key6,
    Key7,
    Key8,
    Key9,
    KeyA,
    KeyB,
    KeyC,
    KeyD,
    KeyE,
    KeyF,
    KeyG,
    KeyH,
    KeyI,
    KeyJ,
    KeyK,
    KeyL,
    KeyM,
    KeyN,
    KeyO,
    KeyP,
    KeyQ,
    KeyR,
    KeyS,
    KeyT,
    KeyU,
    KeyV,
    KeyW,
    KeyX,
    KeyY,
    KeyZ,

    GamepadA,
    GamepadB,
    GamepadX,
    GamepadY,
    GamepadThumbL,
    GamepadThumbR,
    GamepadShoulderL,
    GamepadShoulderR,
    GamepadUp,
    GamepadDown,
    GamepadLeft,
    GamepadRight,
    GamepadBack,
    GamepadStart,
    GamepadGuide,

    ModifierFirst,

    LeftAlt = ModifierFirst,
    RightAlt,
    LeftCtrl,
    RightCtrl,
    LeftShift,
    RightShift,
    LeftMeta,  // "Left GUI" (windows, command (apple), meta)
    RightMeta, // "Right GUI" (windows, command (apple), meta)

    ModifierLast,

    Count = ModifierLast
  };
};

// This should probably be moved to Control.h
struct Control
{
  enum Type
  {
    KEYBOARD,
    MOUSE,
    NONE,
  };

  Type type;
  Key::Enum key;
  ModifierMask modifiers;
  MouseButton::Enum mouseButton;
};

namespace mj
{
  namespace input
  {
    // This should probably be moved to Control.h
    bool GetControl(const Control& control);
    bool GetControlDown(const Control& control);
    bool GetControlUp(const Control& control);

    bool GetKey(Key::Enum key);
    bool GetKeyDown(Key::Enum key);
    bool GetKeyUp(Key::Enum key);

    bool GetMouseButton(MouseButton::Enum button);
    bool GetMouseButtonDown(MouseButton::Enum button);
    bool GetMouseButtonUp(MouseButton::Enum button);

    // Keyboard
#ifdef MJ_INPUT_SDL
    void SetKey(SDL_Scancode key, bool active);
#endif // MJ_INPUT_SDL
    bool IsEscapePressed();
    void SetKeyName(Key::Enum key, const char* keyName);

    const char* GetKeyName(Key::Enum key);
    const char* GetControlName(const Control& control);

    void AddAsciiTyped(char ascii);
    char NextAsciiTyped();

    // Mouse
    void SetMouseButton(MouseButton::Enum button, bool active);
    void AddRelativeMouseMovement(int32_t dx, int32_t dy);
    void GetRelativeMouseMovement(int32_t* dx, int32_t* dy);
    void AddMouseScroll(int32_t scroll);
    int32_t GetMouseScroll();
    bool SetMouseLock(bool locked);
    bool IsMouseLocked();
    void GetMousePosition(float* px, float* py);
    void SetMousePosition(float x, float y);

    Control GetNewControl();

    void Init();
    void Reset();
    void Update();
    void ReleaseEverything();
  } // namespace input
} // namespace mj
