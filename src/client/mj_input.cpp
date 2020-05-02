#include "mj_input.h"
#include <queue>
#include "../../tracy/Tracy.hpp"
#ifdef MJ_INPUT_SDL
#include <SDL_keycode.h>
#endif // MJ_INPUT_SDL

// Multiple of 32 (32-bit integer flags)
constexpr auto INPUT_NUM_KEYS          = Key::Count;
constexpr auto INPUT_NUM_MOUSE_BUTTONS = MouseButton::Count;
constexpr auto INPUT_NUM_INTS          = ((INPUT_NUM_KEYS + 32 - 1) / 32);

static int32_t mouseDXNext;
static int32_t mouseDYNext;
static int32_t mouseDX;
static int32_t mouseDY;

// Keyboard
static uint32_t keyActivePrev[INPUT_NUM_INTS];
static uint32_t keyActive[INPUT_NUM_INTS];
static uint32_t keyMake[INPUT_NUM_INTS];
static uint32_t keyBreak[INPUT_NUM_INTS];

// Key names
static const char* s_keyNames[Key::Count];
static char s_keyNameCache[1024];
static char* s_keyNameNext = s_keyNameCache;

static std::queue<char> s_Ascii;

static ModifierMask modifierMask;

// Mouse
static bool mouseActivePrev[INPUT_NUM_MOUSE_BUTTONS];
static bool mouseActive[INPUT_NUM_MOUSE_BUTTONS];
static bool mouseMake[INPUT_NUM_MOUSE_BUTTONS];
static bool mouseBreak[INPUT_NUM_MOUSE_BUTTONS];
static bool s_MouseLock;
static glm::ivec3 s_MousePosition;

/**
 * @brief      Imported from bgfx.
 */
class Gamepad
{
public:
  Gamepad();

  void reset();
  void setAxis(GamepadAxis::Enum _axis, int32_t _value);
  int32_t getAxis(GamepadAxis::Enum _axis);

private:
  int32_t m_axis[GamepadAxis::Count];
};

/**
 * @brief      { struct_description }
 */
struct GamepadState
{
  GamepadState()
  {
    memset(m_axis, 0, sizeof(m_axis));
  }

  int32_t m_axis[GamepadAxis::Count];
};

// static Gamepad m_gamepad[ENTRY_CONFIG_MAX_GAMEPADS];

/**
 * @brief      Constructs the object.
 */
Gamepad::Gamepad()
{
  reset();
}

/**
 * @brief      { function_description }
 */
void Gamepad::reset()
{
  memset(m_axis, 0, sizeof(m_axis));
}

/**
 * @brief      Sets the axis.
 *
 * @param[in]  _axis   The axis
 * @param[in]  _value  The value
 */
void Gamepad::setAxis(GamepadAxis::Enum _axis, int32_t _value)
{
  m_axis[_axis] = _value;
}

/**
 * @brief      Gets the axis.
 *
 * @param[in]  _axis  The axis
 *
 * @return     The axis.
 */
int32_t Gamepad::getAxis(GamepadAxis::Enum _axis)
{
  return m_axis[_axis];
}

/**
 * @brief      Gets the control.
 *
 * @param[in]  control  The control
 *
 * @return     The control.
 */
bool mj::input::GetControl(const Control& control)
{
  assert(control.key >= 0);
  assert(control.key < INPUT_NUM_KEYS);
  bool key           = ((keyActive[control.key / 32] & (1 << (control.key % 32))) != 0);
  bool modifiersDown = ((modifierMask.all & control.modifiers.all) == control.modifiers.all);
  return key && modifiersDown;
}

/**
 * @brief      Gets the control down.
 *
 * @param[in]  control  The control
 *
 * @return     The control down.
 */
bool mj::input::GetControlDown(const Control& control)
{
  assert(control.key >= 0);
  assert(control.key < INPUT_NUM_KEYS);
  bool keyDown       = ((keyMake[control.key / 32] & (1 << (control.key % 32))) != 0);
  bool modifiersDown = ((modifierMask.all & control.modifiers.all) == control.modifiers.all);
  return keyDown && modifiersDown;
}

/**
 * @brief      Gets the control up.
 *
 * @param[in]  control  The control
 *
 * @return     The control up.
 */
bool mj::input::GetControlUp(const Control& control)
{
  assert(control.key >= 0);
  assert(control.key < INPUT_NUM_KEYS);
  bool keyUp         = ((keyBreak[control.key / 32] & (1 << (control.key % 32))) != 0);
  bool modifiersDown = ((modifierMask.all & control.modifiers.all) == control.modifiers.all);
  return keyUp && modifiersDown;
}

/**
 * @brief      Checks if a key is currently held down.
 *
 * @param[in]  key   The key
 *
 * @return     True if a key is currently held down.
 */
bool mj::input::GetKey(Key::Enum key)
{
  assert(key >= 0);
  assert(key < INPUT_NUM_KEYS);
  return ((keyActive[key / 32] & (1 << (key % 32))) != 0);
}

/**
 * @brief      Checks if a key was pressed on this frame.
 *
 * @param[in]  key   The key
 *
 * @return     True if a key was pressed on this frame.
 */
bool mj::input::GetKeyDown(Key::Enum key)
{
  assert(key >= 0);
  assert(key < INPUT_NUM_KEYS);
  return ((keyMake[key / 32] & (1 << (key % 32))) != 0);
}

/**
 * @brief      Checks if a key was released on this frame.
 *
 * @param[in]  key   The key
 *
 * @return     True if a key was released on this frame.
 */
bool mj::input::GetKeyUp(Key::Enum key)
{
  assert(key >= 0);
  assert(key < INPUT_NUM_KEYS);
  return ((keyBreak[key / 32] & (1 << (key % 32))) != 0);
}

/**
 * @brief      Game thread event pump calls this function to set key state.
 *
 * @param[in]  key     The key
 * @param[in]  active  True if this was a key press event, otherwise false
 */
static void SetKey(Key::Enum key, bool active)
{
  assert(key >= 0);
  assert(key < INPUT_NUM_KEYS);

  if (active)
  {
    keyActive[key / 32] |= (1 << (key % 32));
  }
  else
  {
    keyActive[key / 32] &= ~(1 << (key % 32));
  }

  // Modifiers
  switch (key)
  {
  case Key::LeftAlt:
  {
    modifierMask.LeftAlt = active;
  }
  break;
  case Key::RightAlt:
  {
    modifierMask.RightAlt = active;
  }
  break;
  case Key::LeftCtrl:
  {
    modifierMask.LeftCtrl = active;
  }
  break;
  case Key::RightCtrl:
  {
    modifierMask.RightCtrl = active;
  }
  break;
  case Key::LeftShift:
  {
    modifierMask.LeftShift = active;
  }
  break;
  case Key::RightShift:
  {
    modifierMask.RightShift = active;
  }
  break;
  case Key::LeftMeta:
  {
    modifierMask.LeftMeta = active;
  }
  break;
  case Key::RightMeta:
  {
    modifierMask.RightMeta = active;
  }
  break;
  default:
    break;
  }
}

// One massive jump table
#ifdef MJ_INPUT_SDL
static Key::Enum MapKey(SDL_Keycode key)
{
  switch (key)
  {
  case SDLK_ESCAPE:
    return Key::Esc;
  case SDLK_RETURN:
    return Key::Return;
  case SDLK_TAB:
    return Key::Tab;
  case SDLK_SPACE:
    return Key::Space;
  case SDLK_BACKSPACE:
    return Key::Backspace;
  case SDLK_UP:
    return Key::Up;
  case SDLK_DOWN:
    return Key::Down;
  case SDLK_LEFT:
    return Key::Left;
  case SDLK_RIGHT:
    return Key::Right;
  case SDLK_INSERT:
    return Key::Insert;
  case SDLK_DELETE:
    return Key::Delete;
  case SDLK_HOME:
    return Key::Home;
  case SDLK_END:
    return Key::End;
  case SDLK_PAGEUP:
    return Key::PageUp;
  case SDLK_PAGEDOWN:
    return Key::PageDown;
  case SDLK_PRINTSCREEN:
    return Key::Print;
  case SDLK_PLUS:
    return Key::Plus;
  case SDLK_MINUS:
    return Key::Minus;
  case SDLK_LEFTBRACKET:
    return Key::LeftBracket;
  case SDLK_RIGHTBRACKET:
    return Key::RightBracket;
  case SDLK_SEMICOLON:
    return Key::Semicolon;
  case SDLK_QUOTE:
    return Key::Quote;
  case SDLK_COMMA:
    return Key::Comma;
  case SDLK_PERIOD:
    return Key::Period;
  case SDLK_SLASH:
    return Key::Slash;
  case SDLK_BACKSLASH:
    return Key::Backslash;
  case SDLK_F1:
    return Key::F1;
  case SDLK_F2:
    return Key::F2;
  case SDLK_F3:
    return Key::F3;
  case SDLK_F4:
    return Key::F4;
  case SDLK_F5:
    return Key::F5;
  case SDLK_F6:
    return Key::F6;
  case SDLK_F7:
    return Key::F7;
  case SDLK_F8:
    return Key::F8;
  case SDLK_F9:
    return Key::F9;
  case SDLK_F10:
    return Key::F10;
  case SDLK_F11:
    return Key::F11;
  case SDLK_F12:
    return Key::F12;
  case SDLK_KP_0:
    return Key::NumPad0;
  case SDLK_KP_1:
    return Key::NumPad1;
  case SDLK_KP_2:
    return Key::NumPad2;
  case SDLK_KP_3:
    return Key::NumPad3;
  case SDLK_KP_4:
    return Key::NumPad4;
  case SDLK_KP_5:
    return Key::NumPad5;
  case SDLK_KP_6:
    return Key::NumPad6;
  case SDLK_KP_7:
    return Key::NumPad7;
  case SDLK_KP_8:
    return Key::NumPad8;
  case SDLK_KP_9:
    return Key::NumPad9;
  case SDLK_0:
    return Key::Key0;
  case SDLK_1:
    return Key::Key1;
  case SDLK_2:
    return Key::Key2;
  case SDLK_3:
    return Key::Key3;
  case SDLK_4:
    return Key::Key4;
  case SDLK_5:
    return Key::Key5;
  case SDLK_6:
    return Key::Key6;
  case SDLK_7:
    return Key::Key7;
  case SDLK_8:
    return Key::Key8;
  case SDLK_9:
    return Key::Key9;
  case SDLK_a:
    return Key::KeyA;
  case SDLK_b:
    return Key::KeyB;
  case SDLK_c:
    return Key::KeyC;
  case SDLK_d:
    return Key::KeyD;
  case SDLK_e:
    return Key::KeyE;
  case SDLK_f:
    return Key::KeyF;
  case SDLK_g:
    return Key::KeyG;
  case SDLK_h:
    return Key::KeyH;
  case SDLK_i:
    return Key::KeyI;
  case SDLK_j:
    return Key::KeyJ;
  case SDLK_k:
    return Key::KeyK;
  case SDLK_l:
    return Key::KeyL;
  case SDLK_m:
    return Key::KeyM;
  case SDLK_n:
    return Key::KeyN;
  case SDLK_o:
    return Key::KeyO;
  case SDLK_p:
    return Key::KeyP;
  case SDLK_q:
    return Key::KeyQ;
  case SDLK_r:
    return Key::KeyR;
  case SDLK_s:
    return Key::KeyS;
  case SDLK_t:
    return Key::KeyT;
  case SDLK_u:
    return Key::KeyU;
  case SDLK_v:
    return Key::KeyV;
  case SDLK_w:
    return Key::KeyW;
  case SDLK_x:
    return Key::KeyX;
  case SDLK_y:
    return Key::KeyY;
  case SDLK_z:
    return Key::KeyZ;
  case SDLK_LALT:
    return Key::LeftAlt;
  case SDLK_RALT:
    return Key::RightAlt;
  case SDLK_LCTRL:
    return Key::LeftCtrl;
  case SDLK_RCTRL:
    return Key::RightCtrl;
  case SDLK_LSHIFT:
    return Key::LeftShift;
  case SDLK_RSHIFT:
    return Key::RightShift;
  case SDLK_LGUI:
    return Key::LeftMeta;
  case SDLK_RGUI:
    return Key::RightMeta;
  default:
    return Key::None;
  }
}

void mj::input::SetKey(SDL_Keycode key, bool active)
{
  Key::Enum mappedKey = MapKey(key);
  if (mappedKey != Key::None)
  {
    SetKey(mappedKey, active);
  }
}
#endif // MJ_INPUT_SDL

/**
 * @brief      Queue an ASCII character for ImGui typing.
 *
 * @param[in]  ascii  ASCII character
 */
void mj::input::AddAsciiTyped(char ascii)
{
  s_Ascii.push(ascii);
}

/**
 * @brief      Get the next typed ASCII character. Should only be used by ImGui.
 *
 * @return     Next typed ASCII character
 */
char mj::input::NextAsciiTyped()
{
  char result = '\0';
  if (!s_Ascii.empty())
  {
    result = s_Ascii.front();
    s_Ascii.pop();
  }
  return result;
}

// Mouse

/**
 * @brief      Gets the mouse button.
 *
 * @param[in]  button  The button
 *
 * @return     The mouse button.
 */
bool mj::input::GetMouseButton(MouseButton::Enum button)
{
  assert(button >= 0);
  assert(button < INPUT_NUM_MOUSE_BUTTONS);
  return mouseActive[button];
}

/**
 * @brief      Gets the mouse button down.
 *
 * @param[in]  button  The button
 *
 * @return     The mouse button down.
 */
bool mj::input::GetMouseButtonDown(MouseButton::Enum button)
{
  assert(button >= 0);
  assert(button < INPUT_NUM_MOUSE_BUTTONS);
  return mouseMake[button];
}

/**
 * @brief      Gets the mouse button up.
 *
 * @param[in]  button  The button
 *
 * @return     The mouse button up.
 */
bool mj::input::GetMouseButtonUp(MouseButton::Enum button)
{
  assert(button >= 0);
  assert(button < INPUT_NUM_MOUSE_BUTTONS);
  return mouseBreak[button];
}

/**
 * @brief      Sets the mouse button.
 *
 * @param[in]  button  The button
 * @param[in]  active  The active
 */
void mj::input::SetMouseButton(MouseButton::Enum button, bool active)
{
  assert(button >= 0);
  assert(button < INPUT_NUM_MOUSE_BUTTONS);
  mouseActive[button] = active;
}

/**
 * @brief      Sets the mouse lock.
 *
 * @param[in]  lock  The lock
 *
 * @return     True if the setting was changed.
 */
bool mj::input::SetMouseLock(bool lock)
{
  if (lock != s_MouseLock)
  {
    // entry::setMouseLock({ 0 }, lock);
    s_MouseLock = lock;
    return true;
  }
  return false;
}

/**
 * @brief      Determines if mouse locked.
 *
 * @return     True if mouse locked, False otherwise.
 */
bool mj::input::IsMouseLocked()
{
  return s_MouseLock;
}

/**
 * @brief      { function_description }
 */
void mj::input::Init()
{
  ReleaseEverything();
}

/**
 * @brief      { function_description }
 */
void mj::input::Reset()
{
#if 0
  for (uint32_t ii = 0; ii < BX_COUNTOF(m_gamepad); ++ii)
  {
    m_gamepad[ii].reset();
  }
#endif
}

/**
 * @brief      Called after input polling
 */
void mj::input::Update()
{
  ZoneScoped;
  mouseDX     = mouseDXNext;
  mouseDY     = mouseDYNext;
  mouseDXNext = 0;
  mouseDYNext = 0;

  for (int32_t i = 0; i < INPUT_NUM_INTS; i++)
  {
    // Compare key active state with previous frame
    uint32_t changes = keyActive[i] ^ keyActivePrev[i];
    // Make if key changed and is now active
    keyMake[i] = changes & keyActive[i];

    // Break if key changed and is now inactive
    keyBreak[i] = changes & ~keyActive[i];
  }

  for (int32_t i = 0; i < INPUT_NUM_MOUSE_BUTTONS; i++)
  {
    bool change  = mouseActive[i] ^ mouseActivePrev[i];
    mouseMake[i] = change && mouseActive[i];

    mouseBreak[i] = change && !mouseActive[i];
  }

  // Copy current active state for next frame
  memcpy(keyActivePrev, keyActive, sizeof(keyActivePrev));
  memcpy(mouseActivePrev, mouseActive, sizeof(mouseActivePrev));
}

/**
 * @brief      { function_description }
 */
void mj::input::ReleaseEverything()
{
  // Keyboard
  memset(keyActive, 0, sizeof(keyActive));
  memset(keyActivePrev, 0, sizeof(keyActivePrev));
  memset(keyBreak, 0, sizeof(keyBreak));
  memset(keyMake, 0, sizeof(keyMake));

  // Mouse
  memset(mouseActive, 0, sizeof(mouseActive));
  memset(mouseActivePrev, 0, sizeof(mouseActivePrev));
  memset(mouseBreak, 0, sizeof(mouseBreak));
  memset(mouseMake, 0, sizeof(mouseMake));
  mouseDX     = 0;
  mouseDY     = 0;
  mouseDXNext = 0;
  mouseDYNext = 0;
}

/**
 * @brief      Adds a relative mouse movement.
 *
 * @param[in]  dx    { parameter_description }
 * @param[in]  dy    { parameter_description }
 */
void mj::input::AddRelativeMouseMovement(int32_t dx, int32_t dy)
{
  mouseDXNext += dx;
  mouseDYNext += dy;
}

/**
 * @brief      Gets the relative mouse movement.
 *
 * @param      dx    { parameter_description }
 * @param      dy    { parameter_description }
 */
void mj::input::GetRelativeMouseMovement(int32_t* dx, int32_t* dy)
{
  *dx = mouseDX;
  *dy = mouseDY;
}

/**
 * @brief      Determines if escape pressed.
 *
 * @return     True if escape pressed, False otherwise.
 */
bool mj::input::IsEscapePressed()
{
  return GetKeyDown(Key::Esc);
}

#if 0
/**
 * @brief      Sets a key name.
 *
 * @param[in]  key      The key
 * @param[in]  keyName  The key name (pointer to static SDL memory)
 */
void mj::input::SetKeyName(Key::Enum key, const char* keyName)
{
  s_keyNames[key] = s_keyNameNext;
  strcpy(s_keyNameNext, keyName);
  s_keyNameNext += strlen(s_keyNameNext) + 1; // Advance pointer (TODO: buffer overrun check)
}
#endif

/**
 * @brief      Gets the key name.
 *
 * @param[in]  key   The key
 *
 * @return     The key name.
 */
const char* mj::input::GetKeyName(Key::Enum key)
{
  return s_keyNames[key];
}

#if 0
/**
 * @brief      Allocates a string and copies it over. Does not copy the trailing zero.
 *
 * @param      alloc  The allocator
 * @param[in]  str    The C string to be copied
 *
 * @return     True if the allocation succeeded and the string was copied
 */
static bool AllocateString(mjm::StackAllocator& alloc, const char* str)
{
  bool success = false;

  size_t len = strlen(str);
  void* ptr = alloc.Allocate(len);
  if (ptr)
  {
    memcpy(ptr, str, len);
    success = true;
  }

  return success;
}
#endif

#if 0
/**
 * @brief      Gets the control name. The string is allocated for a single frame: Do not store the pointer.
 *
 * @param[in]  control  The control
 *
 * @return     The control name.
 */
const char* mj::input::GetControlName(const Control& control)
{
  auto& alloc = mjm::GetFrameAllocator();
  void* string = alloc.GetTop();
  bool first = true;

  auto AllocateString = [&](const char* str)
  {
    size_t len = strlen(str);
    void* ptr = alloc.Allocate(len);
    if (ptr)
    {
      memcpy(ptr, str, len);
    }
  };

  auto AddModifierStringIfUsed = [&](uint8_t modifier, Key::Enum key)
  {
    if (modifier)
    {
      if (!first)
      {
        AllocateString(" + ");
      }
      AllocateString(GetKeyName(key));
      first = false;
    }
  };

  AddModifierStringIfUsed(control.modifiers.LeftCtrl,   Key::LeftCtrl);
  AddModifierStringIfUsed(control.modifiers.RightCtrl,  Key::RightCtrl);
  AddModifierStringIfUsed(control.modifiers.LeftAlt,    Key::LeftAlt);
  AddModifierStringIfUsed(control.modifiers.RightAlt,   Key::RightAlt);
  AddModifierStringIfUsed(control.modifiers.LeftShift,  Key::LeftShift);
  AddModifierStringIfUsed(control.modifiers.RightShift, Key::RightShift);
  AddModifierStringIfUsed(control.modifiers.LeftMeta,   Key::LeftMeta);
  AddModifierStringIfUsed(control.modifiers.RightMeta,  Key::RightMeta);

  if (control.type == Control::KEYBOARD)
  {
    if (!first)
    {
      AllocateString(" + ");
    }
    AllocateString(GetKeyName(control.key));
  }

  // Close string
  if (alloc.GetTop() != string)
  {
    char* last = (char*)alloc.Allocate(1);
    if (last)
    {
      *last = '\0';
    }
    else
    {
      *(alloc.GetTop() - 1) = '\0';
    }
  }

  return (const char*)string;
}
#endif

/**
 * @brief      Sets the mouse position.
 *
 * @param[in]  pos   The position
 */
void mj::input::SetMousePosition(const glm::ivec3& pos)
{
  s_MousePosition = pos;
}

/**
 * @brief      Gets the mouse position.
 *
 * @return     The mouse position.
 */
const glm::ivec3& mj::input::GetMousePosition()
{
  return s_MousePosition;
}

/**
 * @brief      Find a new control bind combination.
 *             If any non-modifier key or mouse button is pressed, it uses that in combination with
 *             the currently held modifier keys. Otherwise, the release of the first modifier key is
 *             used.
 *
 * @return     The new control bind combination.
 */
Control mj::input::GetNewControl()
{
  Control control = { Control::NONE, Key::None, Modifier::None, MouseButton::None };

  // Keyboard
  for (int32_t i = 0; i < Key::ModifierFirst; i++)
  {
    Key::Enum key = (Key::Enum)i;
    if (mj::input::GetKeyDown(key))
    {
      control.type = Control::KEYBOARD;
      control.key  = key;
      break;
    }
  }

  // Mouse
  if (control.type == Control::NONE)
  {
    for (int32_t i = MouseButton::Left; i < MouseButton::Count; i++)
    {
      MouseButton::Enum mouseButton = (MouseButton::Enum)i;
      if (mj::input::GetMouseButtonDown(mouseButton))
      {
        control.type        = Control::MOUSE;
        control.mouseButton = mouseButton;
        break;
      }
    }
  }

  if (control.type == Control::NONE)
  {
    // Check for released modifier keys
    for (int32_t i = Key::ModifierFirst; i < Key::ModifierLast; i++)
    {
      Key::Enum key = (Key::Enum)i;
      if (mj::input::GetKeyUp(key))
      {
        control.type = Control::KEYBOARD;
        control.key  = key;
        break;
      }
    }
  }

  if (control.type != Control::NONE)
  {
    // Add all held modifier keys (unless it was the trigger key)
    if (control.key != Key::LeftAlt && mj::input::GetKey(Key::LeftAlt))
      control.modifiers.LeftAlt = true;
    if (control.key != Key::RightAlt && mj::input::GetKey(Key::RightAlt))
      control.modifiers.RightAlt = true;
    if (control.key != Key::LeftCtrl && mj::input::GetKey(Key::LeftCtrl))
      control.modifiers.LeftCtrl = true;
    if (control.key != Key::RightCtrl && mj::input::GetKey(Key::RightCtrl))
      control.modifiers.RightCtrl = true;
    if (control.key != Key::LeftShift && mj::input::GetKey(Key::LeftShift))
      control.modifiers.LeftShift = true;
    if (control.key != Key::RightShift && mj::input::GetKey(Key::RightShift))
      control.modifiers.RightShift = true;
    if (control.key != Key::LeftMeta && mj::input::GetKey(Key::LeftMeta))
      control.modifiers.LeftMeta = true;
    if (control.key != Key::RightMeta && mj::input::GetKey(Key::RightMeta))
      control.modifiers.RightMeta = true;
  }

  return control;
}
