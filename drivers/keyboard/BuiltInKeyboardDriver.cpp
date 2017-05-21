/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa_prajwal@yahoo.co.in'
 *                                                                          
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *                                                                          
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *                                                                          
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/
 */
#include <BuiltInKeyboardDriver.h>
#include <Display.h>
#include <AsmUtil.h>
#include <PortCom.h>
#include <IrqManager.h>
#include <KeyboardHandler.h>
#include <KBInputHandler.h>

#define KB_STAT_OBF 0x01
#define KB_STAT_IBF 0x02
#define KB_REBOOT	0xFE

static void KBDriver_Handler()
{
  unsigned GPRStack[NO_OF_GPR] ;
  AsmUtil_STORE_GPR(GPRStack) ;
  AsmUtil_SET_KERNEL_DATA_SEGMENTS

//  printf("\nKB IRQ\n") ;
  BuiltInKeyboardDriver::Instance().WaitForRead();
  if(!(PortCom_ReceiveByte(KB_STAT_PORT) & 0x20))
    BuiltInKeyboardDriver::Instance().Process((PortCom_ReceiveByte(KB_DATA_PORT)) & 0xFF);

  IrqManager::Instance().SendEOI(StdIRQ::Instance().KEYBOARD_IRQ);

  AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
  AsmUtil_RESTORE_GPR(GPRStack) ;

  __asm__ __volatile__("LEAVE") ;
  __asm__ __volatile__("IRET") ;
}

static const byte NA_CHAR = 0xFF;
static const byte EXTRA_KEYS = 224;
static const int MAX_KEYBOARD_CHARS = 84;

static const byte Keyboard_GENERIC_KEY_MAP[MAX_KEYBOARD_CHARS] = { NA_CHAR,
  Keyboard_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9',

  '0', '-', '=', Keyboard_BACKSPACE, '\t', 'q', 'w', 'e', 'r',

  't', 'y', 'u', 'i', 'o', 'p', '[', ']', Keyboard_ENTER,

  Keyboard_LEFT_CTRL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k',

  'l', ';', '\'', '`', Keyboard_LEFT_SHIFT, '\\', 'z', 'x', 'c',

  'v', 'b', 'n', 'm', ',', '.', '/', Keyboard_RIGHT_SHIFT, '*',

  Keyboard_LEFT_ALT, ' ', Keyboard_CAPS_LOCK, Keyboard_F1, Keyboard_F2,

  Keyboard_F3, Keyboard_F4, Keyboard_F5, Keyboard_F6, Keyboard_F7,

  Keyboard_F8, Keyboard_F9, Keyboard_F10, NA_CHAR, NA_CHAR, Keyboard_KEY_HOME,

  Keyboard_KEY_UP, Keyboard_KEY_PG_UP, NA_CHAR, Keyboard_KEY_LEFT, NA_CHAR,

  Keyboard_KEY_RIGHT, NA_CHAR, Keyboard_KEY_END, Keyboard_KEY_DOWN,

  Keyboard_KEY_PG_DOWN, Keyboard_KEY_INST, Keyboard_KEY_DEL
};

static const byte Keyboard_GENERIC_SHIFTED_KEY_MAP[MAX_KEYBOARD_CHARS] = { NA_CHAR,
  Keyboard_ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(',

  ')', '_', '+', Keyboard_BACKSPACE, '\t', 'Q', 'W', 'E', 'R',

  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', Keyboard_ENTER,

  Keyboard_LEFT_CTRL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K',

  'L', ':', '"', '~', Keyboard_LEFT_SHIFT, '|', 'Z', 'X', 'C',

  'V', 'B', 'N', 'M', '<', '>', '?', Keyboard_RIGHT_SHIFT, '*',

  Keyboard_LEFT_ALT, ' ', Keyboard_CAPS_LOCK, Keyboard_F1, Keyboard_F2,

  Keyboard_F3, Keyboard_F4, Keyboard_F5, Keyboard_F6, Keyboard_F7,

  Keyboard_F8, Keyboard_F9, Keyboard_F10, NA_CHAR, NA_CHAR, Keyboard_KEY_HOME,

  Keyboard_KEY_UP, Keyboard_KEY_PG_UP, NA_CHAR, Keyboard_KEY_LEFT, NA_CHAR,

  Keyboard_KEY_RIGHT, NA_CHAR, Keyboard_KEY_END, Keyboard_KEY_DOWN,

  Keyboard_KEY_PG_DOWN, Keyboard_KEY_INST, Keyboard_KEY_DEL
};

BuiltInKeyboardDriver::BuiltInKeyboardDriver() : _isShiftKey(false), _isCapsLock(false), _isCtrlKey(false)
{
  IrqManager::Instance().DisableIRQ(StdIRQ::Instance().KEYBOARD_IRQ);
  IrqManager::Instance().RegisterIRQ(StdIRQ::Instance().KEYBOARD_IRQ, (unsigned)&KBDriver_Handler);
  IrqManager::Instance().EnableIRQ(StdIRQ::Instance().KEYBOARD_IRQ);

  PortCom_ReceiveByte(KB_DATA_PORT);

  KC::MDisplay().LoadMessage("Built-In Keyboard Driver Initialization", Success);
}

void BuiltInKeyboardDriver::Process(byte rawKey)
{
  byte kbKey = Decode(rawKey);
  if (kbKey == NA_CHAR)
    return;

  if(!KBInputHandler_Process(kbKey))
  {
    KeyboardHandler::Instance().PutToQueueBuffer(kbKey);
    StdIRQ::Instance().KEYBOARD_IRQ.Signal();
  }
}

byte BuiltInKeyboardDriver::Decode(byte rawKey)
{
  if (rawKey == EXTRA_KEYS)
    return NA_CHAR;

  const bool keyRelease = rawKey & 0x80;
  if (keyRelease)
    rawKey -= 0x80;

  if (rawKey >= MAX_KEYBOARD_CHARS)
    return NA_CHAR;

  byte mappedKey = Keyboard_GENERIC_KEY_MAP[rawKey];

  if (keyRelease)
  {
    if(mappedKey == Keyboard_LEFT_SHIFT || mappedKey == Keyboard_RIGHT_SHIFT)
      _isShiftKey = false;
    else if(mappedKey == Keyboard_LEFT_CTRL)
      _isCtrlKey = false;
  }
  else
  {
    if(mappedKey == Keyboard_LEFT_SHIFT || mappedKey == Keyboard_RIGHT_SHIFT)
      _isShiftKey = true;
    else if(mappedKey == Keyboard_CAPS_LOCK)
      _isCapsLock = !_isCapsLock;
    else if(mappedKey == Keyboard_LEFT_CTRL)
      _isCtrlKey = true;
    else
    {
      if(_isShiftKey)
      {
        if(mappedKey >= 'a' && mappedKey <= 'z' && _isCapsLock)
          return mappedKey;
        return Keyboard_GENERIC_SHIFTED_KEY_MAP[rawKey];
      }
      else
      {
        if(mappedKey >= 'a' && mappedKey <= 'z' && _isCapsLock)
          return Keyboard_GENERIC_SHIFTED_KEY_MAP[rawKey];
        return mappedKey;
      }
    }
  }
  return NA_CHAR;
}

bool BuiltInKeyboardDriver::WaitForWrite()
{
  int iTimeOut = 0xFFFFF ;
  while((iTimeOut > 0) && (PortCom_ReceiveByte(KB_STAT_PORT) & KB_STAT_IBF))
    iTimeOut--;
  return (iTimeOut > 0) ;
}

bool BuiltInKeyboardDriver::WaitForRead()
{
  int iTimeOut = 0xFFFFF ;
  while((iTimeOut > 0) && !(PortCom_ReceiveByte(KB_STAT_PORT) & KB_STAT_OBF))
    iTimeOut--;
  return (iTimeOut > 0) ;
}

void BuiltInKeyboardDriver::Reboot()
{
  WaitForWrite();
  PortCom_SendByte(KB_STAT_PORT, KB_REBOOT);
  __asm__ __volatile__("cli; hlt");
}
