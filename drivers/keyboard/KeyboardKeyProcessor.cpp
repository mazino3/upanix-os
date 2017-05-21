#include <KeyboardKeyProcessor.h>
#include <KeyboardHandler.h>

static const byte Keyboard_USB_GENERIC_KEY_MAP[] = {
  0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',

  'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',

  '3', '4', '5', '6', '7', '8', '9', '0', Keyboard_ENTER, Keyboard_ESC, Keyboard_BACKSPACE,

  '\t', ' ', '-', '=', '[', ']', '\\', 0, ';', '\'', '`', ',', '.', '/', Keyboard_CAPS_LOCK,

  Keyboard_F1, Keyboard_F2, Keyboard_F3, Keyboard_F4, Keyboard_F5, Keyboard_F6, Keyboard_F7,

  Keyboard_F8, Keyboard_F9, Keyboard_F10, Keyboard_F11, Keyboard_F12, 0/*print*/, 0/*scroll*/, 0/*pause*/,

  Keyboard_KEY_INST, Keyboard_KEY_HOME, Keyboard_KEY_PG_UP, Keyboard_KEY_DEL, Keyboard_KEY_END,

  Keyboard_KEY_PG_DOWN, Keyboard_KEY_RIGHT, Keyboard_KEY_LEFT, Keyboard_KEY_DOWN, Keyboard_KEY_UP, Keyboard_KEY_NUM,

  '/', '*', '-', '+', Keyboard_ENTER, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '.', 0, 0, 0, 0,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  Keyboard_LEFT_CTRL, Keyboard_LEFT_SHIFT, Keyboard_LEFT_ALT, 0, Keyboard_RIGHT_CTRL, Keyboard_RIGHT_SHIFT, Keyboard_RIGHT_ALT,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const byte Keyboard_USB_SHIFTED_KEY_MAP[] =
{
  0, 0, 0, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',

  'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',

  '#', '$', '%', '^', '&', '*', '(', ')', Keyboard_ENTER, Keyboard_ESC, Keyboard_BACKSPACE,

  '\t', ' ', '_', '+', '{', '}', '|', 0, ':', '"', '~', '<', '>', '?', Keyboard_CAPS_LOCK,

  Keyboard_F1, Keyboard_F2, Keyboard_F3, Keyboard_F4, Keyboard_F5, Keyboard_F6, Keyboard_F7,

  Keyboard_F8, Keyboard_F9, Keyboard_F10, Keyboard_F11, Keyboard_F12, 0/*print*/, 0/*scroll*/, 0/*pause*/,

  Keyboard_KEY_INST, Keyboard_KEY_HOME, Keyboard_KEY_PG_UP, Keyboard_KEY_DEL, Keyboard_KEY_END,

  Keyboard_KEY_PG_DOWN, Keyboard_KEY_RIGHT, Keyboard_KEY_LEFT, Keyboard_KEY_DOWN, Keyboard_KEY_UP, Keyboard_KEY_NUM,

  '/', '*', '-', '+', Keyboard_ENTER, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '.', 0, 0, 0, 0,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  Keyboard_LEFT_CTRL, Keyboard_LEFT_SHIFT, Keyboard_LEFT_ALT, 0, Keyboard_RIGHT_CTRL, Keyboard_RIGHT_SHIFT, Keyboard_RIGHT_ALT,

  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
