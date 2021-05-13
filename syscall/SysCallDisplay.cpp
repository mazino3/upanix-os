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
# include <SysCall.h>
# include <SysCallDisplay.h>
# include <MultiBoot.h>

byte SysCallDisplay_IsPresent(unsigned uiSysCallID)
{
	return (uiSysCallID > SYS_CALL_DISPLAY_START && uiSysCallID < SYS_CALL_DISPLAY_END) ;
}

void SysCallDisplay_Handle(
__volatile__ int* piRetVal,
__volatile__ unsigned uiSysCallID, 
__volatile__ bool bDoAddrTranslation,
__volatile__ unsigned uiP1, 
__volatile__ unsigned uiP2, 
__volatile__ unsigned uiP3, 
__volatile__ unsigned uiP4, 
__volatile__ unsigned uiP5, 
__volatile__ unsigned uiP6, 
__volatile__ unsigned uiP7, 
__volatile__ unsigned uiP8, 
__volatile__ unsigned uiP9)
{
	switch(uiSysCallID)
	{
		case SYS_CALL_DISPLAY_MESSAGE :
			// P1 => Address of DisplayString relative to processBase 
			// P2 => Color Attr
			{
				char* szMessageAddress = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1);
				KC::MDisplay().Message(szMessageAddress, uiP2) ; 
			}
			break ;

		case SYS_CALL_DISPLAY_CHARACTER :
			// P1 => Character to be displayed
			// P2 => Color Attr
			{
				KC::MDisplay().Character((char)(uiP1), uiP2) ; 
			}
			break ;

		case SYS_CALL_DISPLAY_CLR_SCR :
			{
				KC::MDisplay().ClearScreen() ;
			}
			break ;

		case SYS_CALL_DISPLAY_MOV_CURSOR :
			// P1 => No of Positions to Move
			{
				KC::MDisplay().MoveCursor((int)uiP1) ;
			}
			break ;

		case SYS_CALL_DISPLAY_CLR_LINE :
			// P1 => Position to Clear From
			{
				KC::MDisplay().ClearLine((int)uiP1) ;
			}
			break ;

		case SYS_CALL_DISPLAY_ADDRESS :
			// P1 => Address of DisplayString relative to processBase 
			// P2 => Number to be Displayed
			{
				char* szMessageAddress = KERNEL_ADDR(bDoAddrTranslation, char*, uiP1);
				KC::MDisplay().Address(szMessageAddress, uiP2) ; 
			}
			break ;

		case SYS_CALL_DISPLAY_SET_CURSOR :
			// P1 => Cursor Position
			// P2 => Update Cursor On Screen
			{
				KC::MDisplay().SetCursor((int)uiP1, uiP2) ;
			}
			break ;

		case SYS_CALL_DISPLAY_GET_CURSOR :
			{
				*piRetVal = KC::MDisplay().GetCurrentCursorPosition();
			}
			break ;

		case SYS_CALL_DISPLAY_RAW_CHAR :
			// P1 => Character to be displayed
			// P2 => Color Attr
			// P3 => Update Cursor On Screen
			{
				KC::MDisplay().RawCharacter((char)(uiP1), uiP2, uiP3) ;
			}
			break ;

    case SYS_CALL_DISPLAY_RAW_CHAR_AREA:
      {
        const MChar* src = KERNEL_ADDR(bDoAddrTranslation, MChar*, uiP1);
        KC::MDisplay().RawCharacterArea(src, uiP2, uiP3, (int)uiP4);
      }
      break;

    case SYS_CALL_DISPLAY_CONSOLE_SIZE:
      // P1 => Row size (return)
      // P2 => Column size (return)
      {
        auto maxRows = KERNEL_ADDR(bDoAddrTranslation, unsigned*, uiP1);
        auto maxCols = KERNEL_ADDR(bDoAddrTranslation, unsigned*, uiP2);
        *maxRows = KC::MDisplay().MaxRows();
        *maxCols = KC::MDisplay().MaxColumns();
      }
      break;

	  case SYS_CALL_DISPLAY_FRAMEBUFFER_INFO:
	    // P1 => Return address of FramebufferInfo
    {
      auto framebufferInfo = KERNEL_ADDR(bDoAddrTranslation, FramebufferInfo*, uiP1);
      const auto f = MultiBoot::Instance().VideoFrameBufferInfo();
      framebufferInfo->_pitch = f->framebuffer_pitch;
      framebufferInfo->_width = f->framebuffer_width;
      framebufferInfo->_height = f->framebuffer_height;
      framebufferInfo->_bpp = f->framebuffer_bpp;
    }
    break;
	}
}
