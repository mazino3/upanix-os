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

#include <Global.h>

#define Keyboard_SUCCESS 0
#define Keyboard_ERR_BUFFER_EMPTY 1
#define Keyboard_ERR_BUFFER_FULL 2

typedef enum
{
	Keyboard_ESC = 0,
	Keyboard_BACKSPACE,
	Keyboard_ENTER,
	Keyboard_LEFT_CTRL,
	Keyboard_LEFT_SHIFT,
	Keyboard_RIGHT_SHIFT,
	Keyboard_LEFT_ALT,
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

	Keyboard_KEY_UP = 400,
	Keyboard_KEY_DOWN,
	Keyboard_KEY_LEFT,
	Keyboard_KEY_RIGHT,
	Keyboard_KEY_HOME,
	Keyboard_KEY_END,
	Keyboard_KEY_INST,
	Keyboard_KEY_DEL,
	Keyboard_KEY_PG_UP,
	Keyboard_KEY_PG_DOWN,

} Keyboard_SpecialKeys ;

#define CTRL_VALUE 100
#define CTRL(v, c) (v == CTRL_VALUE + c)

void Keyboard_Initialize() ;
int Keyboard_GetKeyInBlockMode() ;
byte Keyboard_MapKey(char key) ;


#endif

