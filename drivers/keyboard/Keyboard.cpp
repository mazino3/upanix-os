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

void Keyboard_Initialize()
{
	KBDriver::Instance();

	Keyboard_bShiftKey = false ;
	Keyboard_bCapsLosk = false ;
	Keyboard_bCtrlKey = false ;
	Keyboard_bExtraCharSet = false ;
}

static int HandleNormalKey(KeyboardKey kbKey)
{
  if(kbKey.IsKeyRelease())
  {
    if(kbKey.IsKey(Keyboard_LEFT_SHIFT) || kbKey.IsKey(Keyboard_RIGHT_SHIFT))
    {
      Keyboard_bShiftKey = false ;
    }
    else if(kbKey.IsKey(Keyboard_LEFT_CTRL))
    {
      Keyboard_bCtrlKey = false ;
    }
  }
  else if(kbKey.IsNormal())
  {
    int key = kbKey.MapChar();
    int shiftedKey = kbKey.ShiftedMapChar();
    int value = -1;

    if(!kbKey.IsSpecial())
    {
      if(Keyboard_bShiftKey)
      {
        if(shiftedKey >= 'A' && shiftedKey <= 'Z')
        {
          if(Keyboard_bCapsLosk)
            value = key;
          else
            value = shiftedKey;
        }
        else
          value = shiftedKey;
      }
      else
      {
        if(shiftedKey >= 'A' && shiftedKey <= 'Z')
        {
          if(Keyboard_bCapsLosk)
            value = shiftedKey;
          else
            value = key;
        }
        else
          value = key;
      }

      if(Keyboard_bCtrlKey)
      {
        value += CTRL_VALUE ;
      }

      return value ;
    }
    else
    {
      if(kbKey.IsKey(Keyboard_LEFT_SHIFT) || kbKey.IsKey(Keyboard_RIGHT_SHIFT))
      {
        Keyboard_bShiftKey = true ;
      }
      else if(kbKey.IsKey(Keyboard_CAPS_LOCK))
      {
        Keyboard_bCapsLosk = (Keyboard_bCapsLosk) ? false : true ;
      }
      else if(kbKey.IsKey(Keyboard_LEFT_CTRL))
      {
        Keyboard_bCtrlKey = true ;
      }
      else
      {
        return key;
      }
    }
  }

  return -1 ;
}

int Keyboard_GetKeyInBlockMode()
{
	int value ;

	while(SUCCESS)
	{
    auto kbKey = KBDriver::Instance().GetCharInBlockMode();

    if(kbKey.IsExtraKey())
		{
			Keyboard_bExtraCharSet = true ;
			continue ;
		}

		if(Keyboard_bExtraCharSet)
		{
			Keyboard_bExtraCharSet = false ;
      value = kbKey.ExtraKey();
		}
		else // Normal Char set
		{
      value = HandleNormalKey(kbKey);
		}

		if(value < 0)
			continue ;

		return value ;
	}
}
