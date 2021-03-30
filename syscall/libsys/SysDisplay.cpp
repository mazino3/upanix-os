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

void SysDisplay_Message(const char* szMessage, unsigned uiAttr)
{
	int iRetStatus ;
	SysCallDisplay_Handle(&iRetStatus, SYS_CALL_DISPLAY_MESSAGE, false, (unsigned)szMessage, uiAttr, 3, 4, 5, 6, 7, 8, 9) ;
}

void SysDisplay_Character(const char ch, unsigned uiAttr)
{
	__volatile__ unsigned v = ch ;
	int iRetStatus ;
	SysCallDisplay_Handle(&iRetStatus, SYS_CALL_DISPLAY_CHARACTER, false, v, uiAttr, 3, 4, 5, 6, 7, 8, 9);
}

void SysDisplay_ClearScreen()
{
	int iRetStatus ;
	SysCallDisplay_Handle(&iRetStatus, SYS_CALL_DISPLAY_CLR_SCR, false, 1, 2, 3, 4, 5, 6, 7, 8, 9);
}

void SysDisplay_MoveCursor(int n)
{
	int iRetStatus ;
	SysCallDisplay_Handle(&iRetStatus, SYS_CALL_DISPLAY_MOV_CURSOR, false, n, 2, 3, 4, 5, 6, 7, 8, 9);
}

void SysDisplay_ClearLine(int pos)
{
	int iRetStatus ;
	SysCallDisplay_Handle(&iRetStatus, SYS_CALL_DISPLAY_CLR_LINE, false, pos, 2, 3, 4, 5, 6, 7, 8, 9);
}

void SysDisplay_Address(const char* szMessage, unsigned uiNumber)
{
	int iRetStatus ;
	SysCallDisplay_Handle(&iRetStatus, SYS_CALL_DISPLAY_ADDRESS, false, (unsigned)szMessage, uiNumber, 3, 4, 5, 6, 7, 8, 9);
}

void SysDisplay_SetCursor(__volatile__ int iCurPos, __volatile__ bool bUpdateCursorOnScreen)
{
	int iRetStatus ;
	SysCallDisplay_Handle(&iRetStatus, SYS_CALL_DISPLAY_SET_CURSOR, false, iCurPos, bUpdateCursorOnScreen, 3, 4, 5, 6, 7, 8, 9);
}

int SysDisplay_GetCursor()
{
	int iRetStatus ;
	SysCallDisplay_Handle(&iRetStatus, SYS_CALL_DISPLAY_GET_CURSOR, false, 1, 2, 3, 4, 5, 6, 7, 8, 9);
	return iRetStatus ;
}

void SysDisplay_RawCharacter(__volatile__ const char ch, __volatile__ unsigned uiAttr, __volatile__ bool bUpdateCursorOnScreen)
{
	__volatile__ unsigned v = ch ;
	int iRetStatus ;
	SysCallDisplay_Handle(&iRetStatus, SYS_CALL_DISPLAY_RAW_CHAR, false, v, uiAttr, bUpdateCursorOnScreen, 4, 5, 6, 7, 8, 9);
}

void SysDisplay_RawCharacterArea(const MChar* src, uint32_t rows, uint32_t cols, int curPos) {
  int iRetStatus ;
  SysCallDisplay_Handle(&iRetStatus, SYS_CALL_DISPLAY_RAW_CHAR_AREA, false, (uint32_t)src, rows, cols, curPos, 5, 6, 7, 8, 9);
}