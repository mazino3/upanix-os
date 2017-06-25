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
#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

# include <ctype.h>

typedef enum
{
  Keyboard_ESC = 1,
  Keyboard_BACKSPACE,
  Keyboard_ENTER,
  Keyboard_LEFT_CTRL,
  Keyboard_LEFT_SHIFT,
  Keyboard_LEFT_ALT,
  Keyboard_RIGHT_SHIFT,
  Keyboard_RIGHT_CTRL,
  Keyboard_RIGHT_ALT,
  Keyboard_CAPS_LOCK,
  Keyboard_F1 = 19,
  Keyboard_F2,
  Keyboard_F3,
  Keyboard_F4,
  Keyboard_F5,
  Keyboard_F6,
  Keyboard_F7,
  Keyboard_F8,
  Keyboard_F9,
  Keyboard_F10,
  Keyboard_F11,
  Keyboard_F12,

  Keyboard_KEY_UP = 150,
  Keyboard_KEY_DOWN,
  Keyboard_KEY_LEFT,
  Keyboard_KEY_RIGHT,
  Keyboard_KEY_HOME,
  Keyboard_KEY_END,
  Keyboard_KEY_INST,
  Keyboard_KEY_DEL,
  Keyboard_KEY_PG_UP,
  Keyboard_KEY_PG_DOWN,
  Keyboard_KEY_NUM,

  Keyboard_NA_CHAR = 0xFF
} Keyboard_SpecialKeys ;

#define CTRL_VALUE 100
#define CTRL(v, c) (v == CTRL_VALUE + c)

extern void SysKB_GetWait(int* ch) ;
#define kb_get_ch_wait(ch) SysKB_GetWait(ch)

bool is_special_char(char ch) ;

#endif

