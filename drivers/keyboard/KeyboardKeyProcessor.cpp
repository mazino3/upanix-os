#include <KeyboardKeyProcessor.h>
#include <Keyboard.h>

// index 0 is dummy
static __volatile__ byte Keyboard_GENERIC_KEY_MAP[] = { 'x',
  Keyboard_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9',

  '0', '-', '=', Keyboard_BACKSPACE, '\t', 'q', 'w', 'e', 'r',

  't', 'y', 'u', 'i', 'o', 'p', '[', ']', Keyboard_ENTER,

  Keyboard_LEFT_CTRL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k',

  'l', ';', '\'', '`', Keyboard_LEFT_SHIFT, '\\', 'z', 'x', 'c',

  'v', 'b', 'n', 'm', ',', '.', '/', Keyboard_RIGHT_SHIFT, '*',

  Keyboard_LEFT_ALT, ' ', Keyboard_CAPS_LOCK, Keyboard_F1, Keyboard_F2,

  Keyboard_F3, Keyboard_F4, Keyboard_F5, Keyboard_F6, Keyboard_F7,

    Keyboard_F8, Keyboard_F9, Keyboard_F10
} ;

static __volatile__ byte Keyboard_GENERIC_SHIFTED_KEY_MAP[] = { 'x',
  Keyboard_ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(',

  ')', '_', '+', Keyboard_BACKSPACE, '\t', 'Q', 'W', 'E', 'R',

  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', Keyboard_ENTER,

  Keyboard_LEFT_CTRL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K',

  'L', ':', '"', '~', Keyboard_LEFT_SHIFT, '|', 'Z', 'X', 'C',

  'V', 'B', 'N', 'M', '<', '>', '?', Keyboard_RIGHT_SHIFT, '*',

  Keyboard_LEFT_ALT, ' ', Keyboard_CAPS_LOCK, Keyboard_F1, Keyboard_F2,

  Keyboard_F3, Keyboard_F4, Keyboard_F5, Keyboard_F6, Keyboard_F7,

    Keyboard_F8, Keyboard_F9, Keyboard_F10
} ;

// Extra Keys
#define EXTRA_KEYS 224

#define EXTRA_KEY_UP		72
#define EXTRA_KEY_DOWN		80
#define EXTRA_KEY_LEFT		75
#define EXTRA_KEY_RIGHT		77
#define EXTRA_KEY_HOME		71
#define EXTRA_KEY_END		79
#define EXTRA_KEY_INST		82
#define EXTRA_KEY_DEL		83
#define EXTRA_KEY_PG_UP		73
#define EXTRA_KEY_PG_DOWN	81
#define EXTRA_KEY_ENTER		28

byte BuiltInKeyboardKeyProcessor::Data(byte rawKey)
{
  if (IsKeyRelease(rawKey))
    rawKey -= 0x80;
  return rawKey;
}

byte BuiltInKeyboardKeyProcessor::MapChar(byte rawKey)
{
  return Keyboard_GENERIC_KEY_MAP[(int)Data(rawKey)];
}

byte BuiltInKeyboardKeyProcessor::ShiftedMapChar(byte rawKey)
{
  return Keyboard_GENERIC_SHIFTED_KEY_MAP[(int)Data(rawKey)];
}

bool BuiltInKeyboardKeyProcessor::IsKeyRelease(byte rawKey)
{
  return rawKey & 0x80;
}

bool BuiltInKeyboardKeyProcessor::IsExtraKey(byte rawKey)
{
  return MapChar(rawKey) == EXTRA_KEYS;
}

int BuiltInKeyboardKeyProcessor::ExtraKey(byte rawKey)
{
  if(IsKeyRelease(rawKey))
    return -1 ;
  else
  {
    switch(MapChar(rawKey))
    {
    case EXTRA_KEY_UP:
        return Keyboard_KEY_UP ;
    case EXTRA_KEY_DOWN:
        return Keyboard_KEY_DOWN ;
    case EXTRA_KEY_LEFT:
        return Keyboard_KEY_LEFT ;
    case EXTRA_KEY_RIGHT:
        return Keyboard_KEY_RIGHT ;
    case EXTRA_KEY_HOME:
        return Keyboard_KEY_HOME ;
    case EXTRA_KEY_END:
        return Keyboard_KEY_END ;
    case EXTRA_KEY_INST:
        return Keyboard_KEY_INST ;
    case EXTRA_KEY_DEL:
        return Keyboard_KEY_DEL ;
    case EXTRA_KEY_PG_UP:
        return Keyboard_KEY_PG_UP ;
    case EXTRA_KEY_PG_DOWN:
        return Keyboard_KEY_PG_DOWN ;
    case EXTRA_KEY_ENTER:
        return Keyboard_ENTER ;
    }
  }
  return -1;
}

bool BuiltInKeyboardKeyProcessor::IsKey(byte rawKey, byte val)
{
  return MapChar(rawKey) == val;
}

bool BuiltInKeyboardKeyProcessor::IsNormal(byte rawKey)
{
  return rawKey >= 0 && rawKey <= 69;
}

bool BuiltInKeyboardKeyProcessor::IsSpecial(byte rawKey)
{
  auto key = MapChar(rawKey) ;

  return (key == Keyboard_ESC ||
      key == Keyboard_BACKSPACE ||
      key == Keyboard_LEFT_CTRL ||
      key == Keyboard_LEFT_SHIFT ||
      key == Keyboard_RIGHT_SHIFT ||
      key == Keyboard_LEFT_ALT ||
      key == Keyboard_CAPS_LOCK ||
      key == Keyboard_F1 ||
      key == Keyboard_F2 ||
      key == Keyboard_F3 ||
      key == Keyboard_F4 ||
      key == Keyboard_F5 ||
      key == Keyboard_F6 ||
      key == Keyboard_F7 ||
      key == Keyboard_F8 ||
      key == Keyboard_F9 ||
      key == Keyboard_F10);
}
