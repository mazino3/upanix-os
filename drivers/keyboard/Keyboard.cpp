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
#include <Keyboard.h>
#include <KBDriver.h>

__volatile__ byte Keyboard_bShiftKey ;
__volatile__ byte Keyboard_bCapsLosk ;
__volatile__ byte Keyboard_bCtrlKey ;
__volatile__ byte Keyboard_bExtraCharSet ;

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

/******* static functions *******/
static byte Keyboard_IsSpecial(__volatile__ char key)
{
	key = Keyboard_GENERIC_KEY_MAP[(int)key] ;

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
			key == Keyboard_F10) ;
}

static byte Keyboard_IsKey(byte key, byte val)
{
	return (Keyboard_GENERIC_KEY_MAP[(int)key] == val) ;
}

static int Keyboard_HandleNormalKeys(int key)
{
	int value ;
	if(key >= 0 && key <= 69)
	{
		if(!Keyboard_IsSpecial(key))
		{
			if(Keyboard_bShiftKey)
			{
				if(Keyboard_GENERIC_SHIFTED_KEY_MAP[(int)key] >= 'A' && 
					Keyboard_GENERIC_SHIFTED_KEY_MAP[(int)key] <= 'Z')
				{
					if(Keyboard_bCapsLosk)
						value = Keyboard_GENERIC_KEY_MAP[(int)key] ;
					else
						value = Keyboard_GENERIC_SHIFTED_KEY_MAP[(int)key] ;
				}
				else
					value = Keyboard_GENERIC_SHIFTED_KEY_MAP[(int)key] ;
			}
			else
			{
				if(Keyboard_GENERIC_SHIFTED_KEY_MAP[(int)key] >= 'A' && 
					Keyboard_GENERIC_SHIFTED_KEY_MAP[(int)key] <= 'Z')
				{
					if(Keyboard_bCapsLosk)
						value = Keyboard_GENERIC_SHIFTED_KEY_MAP[(int)key] ;
					else
						value = Keyboard_GENERIC_KEY_MAP[(int)key] ;
				}
				else
					value = Keyboard_GENERIC_KEY_MAP[(int)key] ;
			}

			if(Keyboard_bCtrlKey)
			{
				value += CTRL_VALUE ;
			}

			return value ;
		}
		else
		{
			if(Keyboard_IsKey(key, Keyboard_LEFT_SHIFT) || 
				Keyboard_IsKey(key, Keyboard_RIGHT_SHIFT))
			{
				Keyboard_bShiftKey = true ;
			}
			else if(Keyboard_IsKey(key, Keyboard_CAPS_LOCK))
			{
				Keyboard_bCapsLosk = (Keyboard_bCapsLosk) ? false : true ;
			}
			else if(Keyboard_IsKey(key, Keyboard_LEFT_CTRL))
			{
				Keyboard_bCtrlKey = true ;
			}
			else
			{
				return Keyboard_GENERIC_KEY_MAP[(int)key] ;
			}
		}
	}
	else if(key & 0x80)
	{
		key -= 0x80 ;

		if(Keyboard_IsKey(key, Keyboard_LEFT_SHIFT) || 
			Keyboard_IsKey(key, Keyboard_RIGHT_SHIFT))
		{
			Keyboard_bShiftKey = false ;
		}
		else if(Keyboard_IsKey(key, Keyboard_LEFT_CTRL))
		{
			Keyboard_bCtrlKey = false ;
		}
//		else if(Keyboard_IsSpecial(key))
//			return Keyboard_GENERIC_KEY_MAP[(int)key] ;
	}

	return -1 ;
}

static int Keyboard_HandleExtraKeys(int key)
{
	if(key & 0x80)
		return -1 ;
	else
	{
		switch(key)
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
	return -1 ;
}

/*********************************************************/

void Keyboard_Initialize()
{
	KBDriver_Initialize() ;

	Keyboard_bShiftKey = false ;
	Keyboard_bCapsLosk = false ;
	Keyboard_bCtrlKey = false ;
	Keyboard_bExtraCharSet = false ;
}

byte Keyboard_MapKey(char key)
{
	return Keyboard_GENERIC_KEY_MAP[(int)key] ;
}

int Keyboard_GetKeyInBlockMode()
{
	byte key ;
	int value ;

	while(SUCCESS)
	{
		KBDriver_GetCharInBlockMode(&key) ;

		if(key == EXTRA_KEYS)
		{
			Keyboard_bExtraCharSet = true ;
			continue ;
		}

		if(Keyboard_bExtraCharSet)
		{
			Keyboard_bExtraCharSet = false ;
			value = Keyboard_HandleExtraKeys(key) ;
		}
		else // Normal Char set
		{
			value = Keyboard_HandleNormalKeys(key) ;
		}

		if(value < 0)
			continue ;

		return value ;
	}
}
