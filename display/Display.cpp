/*
 *	Mother Operating System - An x86 based Operating System
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
#include <Display.h>
#include <StringUtil.h>

Display::Attribute::Attribute() : 
	m_blink(DisplayManager::NO_BLINK), 
	m_fgColor(DisplayManager::FG_WHITE),
	m_bgColor(DisplayManager::BG_BLACK)
{
	UpdateAttrVal() ;
}

Display::Attribute::Attribute(const DisplayManager::FGColor& fgColor, const DisplayManager::BGColor& bgColor) :
	m_blink(DisplayManager::NO_BLINK),
	m_fgColor(fgColor),
	m_bgColor(bgColor)
{
	UpdateAttrVal() ;
}

Display::Attribute::Attribute(const DisplayManager::Blink& blink, const DisplayManager::FGColor& fgColor, const DisplayManager::BGColor& bgColor) :
	m_blink(blink),
	m_fgColor(fgColor),
	m_bgColor(bgColor)
{
	UpdateAttrVal() ;
}

Display::Attribute::Attribute(const byte& rawAttr)
{
	m_Attr = rawAttr ;
	m_blink = static_cast<DisplayManager::Blink>(rawAttr & 0x80) ;
	m_fgColor = static_cast<DisplayManager::FGColor>(rawAttr & DisplayManager::FG_BRIGHT_WHITE) ;
	m_bgColor = static_cast<DisplayManager::BGColor>(rawAttr & DisplayManager::BG_WHITE) ;
}

void Display::Attribute::SetBlink(const DisplayManager::Blink& blink)
{
	m_blink = blink ;
	UpdateAttrVal() ;
}

void Display::Attribute::SetFGColor(const DisplayManager::FGColor& fgColor)
{
	m_fgColor = fgColor ;
	UpdateAttrVal() ;
}

void Display::Attribute::SetBGColor(const DisplayManager::BGColor& bgColor)
{
	m_bgColor = bgColor ;
	UpdateAttrVal() ;
}

void Display::Attribute::UpdateAttrVal()
{
	m_Attr = m_blink | m_fgColor | m_bgColor ;
}

Display::Display() : 
	m_iMaxRows(DisplayManager::NO_ROWS), 
	m_iMaxCols(DisplayManager::NO_COLUMNS) 
{
	Message("\n Display Constructor Called") ;
}

Display::Display(const int& nMaxRows, const int& nMaxCols) :
	m_iMaxRows(nMaxRows),
	m_iMaxCols(nMaxCols)
{
}

int Display::ClearScreen()
{
	unsigned int noOfDisplayBytes = m_iMaxRows * m_iMaxCols * DisplayManager::NO_BYTES_PER_CHARACTER ;
	unsigned int i ;

	for(i = 0; i < noOfDisplayBytes; i += 2)
	{
		DisplayManager::PutChar(i, ' ') ;
		DisplayManager::PutChar(i + 1, WHITE_ON_BLACK()) ;
	}

	DisplayManager::UpdateCursorPosition(0, true) ;

	return 0 ;
}

int Display::NextLine()
{
	int curLine = (DisplayManager::GetCurrentCursorPosition() / m_iMaxCols) + 1 ;

	if(curLine < m_iMaxRows)
	{
		DisplayManager::UpdateCursorPosition(curLine * m_iMaxCols, true) ;
	}
	else
	{
		DisplayManager::SetCurrentCursorPosition(m_iMaxCols * m_iMaxRows) ;
	}

	ScrollDown() ;

	return 0 ;
}

void Display::ScrollDown()
{
	unsigned int noOfDisplayBytes = (m_iMaxRows - 1) * m_iMaxCols * DisplayManager::NO_BYTES_PER_CHARACTER;
	unsigned int i;
	unsigned int offset = m_iMaxCols * DisplayManager::NO_BYTES_PER_CHARACTER ;
	
	if(!(DisplayManager::GetCurrentCursorPosition() >= m_iMaxRows * m_iMaxCols))
		return ;

	for(i = 0; i < noOfDisplayBytes; i++)
	{
		DisplayManager::PutChar(i, DisplayManager::GetChar(i + offset)) ;
	}

	while(i < noOfDisplayBytes + m_iMaxCols * DisplayManager::NO_BYTES_PER_CHARACTER)
	{
		DisplayManager::PutChar(i++, ' ') ;
		DisplayManager::PutChar(i++, WHITE_ON_BLACK()) ;
	}

	DisplayManager::UpdateCursorPosition(noOfDisplayBytes / 2, true) ;
}

int Display::Message(const __volatile__ char* message, const Display::Attribute& attr)
{
	__volatile__ register unsigned int i ;

	for(i = 0; message[i] != '\0'; i++)
		Character(message[i], attr) ;

	return 0;
}

int Display::Message(const __volatile__ char* message, const byte& rawAttr)
{
	__volatile__ register unsigned int i ;

	Attribute attr(rawAttr) ;

	for(i = 0; message[i] != '\0'; i++)
		Character(message[i], attr) ;

	return 0;
}

int Display::nMessage(const __volatile__ char* message, int len, const Display::Attribute& attr)
{
	for(int i = 0; i < len; i++)
		Character(message[i], attr) ;

	return 0;
}

void Display::Address(const char *message, unsigned int address)
{
	__volatile__ char addressDigit[9] ;
	__volatile__ byte i ;

	for(i = 0; i < 8; i++)
	{
		addressDigit[7 - i] = (char)((address >> 4*i) & 0x0F) ;

		if(addressDigit[7 - i] < 10)
			addressDigit[7 - i] += 0x30 ;
		else
			addressDigit[7 - i] += (0x41 - 0x0A) ;
	}

	addressDigit[8] = '\0' ;

	for(i = 0; i < 7; i++)
		if(addressDigit[i] != '0')
			break ;

	unsigned j ;
	for(j = 0; i < 9; i++, j++)
		addressDigit[j] = addressDigit[i] ;

	Message(message, WHITE_ON_BLACK()) ;
	Message(addressDigit, WHITE_ON_BLACK()) ;
}

void Display::Number(const char *message, DWORD dwNumber)
{
	__volatile__ char szNumber[21] ;
	__volatile__ byte i ;

	for(i = 0; i < 20; i++)
	{
		szNumber[i] = (dwNumber % 10) + 0x30 ;
		dwNumber = dwNumber / 10 ;
		if(!dwNumber)
			break ;
	}

	i++ ;

	szNumber[i] = '\0' ;

	String_Reverse((char*)szNumber) ;

	Message(message, WHITE_ON_BLACK()) ;
	Message(szNumber, WHITE_ON_BLACK()) ;
}

void Display::DDNumberInHex(const char *message, DDWORD ddNumber)
{
	__volatile__ char addressDigit[17] ;
	__volatile__ byte i ;

	for(i = 0; i < 16; i++)
	{
		addressDigit[15 - i] = (char)((ddNumber >> 4*i) & 0x0F) ;

		if(addressDigit[15 - i] < 10)
			addressDigit[15 - i] += 0x30 ;
		else
			addressDigit[15 - i] += (0x41 - 0x0A) ;
	}

	addressDigit[16] = '\0' ;

	for(i = 0; i < 15; i++)
		if(addressDigit[i] != '0')
			break ;

	unsigned j ;
	for(j = 0; i < 17; i++, j++)
		addressDigit[j] = addressDigit[i] ;

	Message(message, WHITE_ON_BLACK()) ;
	Message(addressDigit, WHITE_ON_BLACK()) ;
}

void Display::DDNumberInDec(const char *message, DDWORD ddNumber)
{
	__volatile__ char addressDigit[17] ;
	__volatile__ byte i ;

	for(i = 0; i < 16; i++)
	{
		addressDigit[15 - i] = (char)((ddNumber >> 4*i) & 0x0F) ;

		if(addressDigit[15 - i] < 10)
			addressDigit[15 - i] += 0x30 ;
		else
			addressDigit[15 - i] += (0x41 - 0x0A) ;
	}

	addressDigit[16] = '\0' ;

	for(i = 0; i < 15; i++)
		if(addressDigit[i] != '0')
			break ;

	unsigned j ;
	for(j = 0; i < 17; i++, j++)
		addressDigit[j] = addressDigit[i] ;

	Message(message, WHITE_ON_BLACK()) ;
	Message(addressDigit, WHITE_ON_BLACK()) ;
}

void Display::Character(char ch, const Display::Attribute& attr)
{
	if(ch == '\n')
	{
		NextLine() ;
	}
	else if(ch == '\t')
	{
		int i ;
		for(i = 0; i < 4; i++)
			Character(' ', attr) ;
	}
	else
	{
		DisplayManager::PutChar(DisplayManager::GetCurrentDisplayBytePosition() + 0, ch) ;
		DisplayManager::PutChar(DisplayManager::GetCurrentDisplayBytePosition() + 1, attr) ;

		DisplayManager::UpdateCursorPosition(DisplayManager::GetCurrentCursorPosition() + 1, true) ;

		ScrollDown() ;
	}
}

void Display::ClearLine(int iStartPos)
{
	if(iStartPos == Display::START_CURSOR_POS)
		iStartPos = DisplayManager::GetCurrentCursorPosition() ;
	else if(!(iStartPos >= 0 && iStartPos < m_iMaxCols))
		iStartPos = DisplayManager::GetCurrentCursorPosition() ;

	int i ;
	for(i = iStartPos * 2; (i % (m_iMaxCols * 2)) != 0; i += 2)
	{
		DisplayManager::PutChar(i + 0, ' ') ;
		DisplayManager::PutChar(i + 1, WHITE_ON_BLACK()) ;
	}
}

void Display::LoadMessage(const char* loadMessage, Result result)
{
	byte width = 50 ;
	unsigned int i ;
	char spaces[50] = "\0" ;
	
	for(i = 0; loadMessage[i] != '\0'; i++) ;

	width = width - i ;
	
	for(i = 0; i < width; i++)
			spaces[i] = ' ' ;
	spaces[i] = '\0' ;
	
	Message("\n ", WHITE_ON_BLACK()) ;
	Message(loadMessage, WHITE_ON_BLACK()) ;
	Message(spaces, WHITE_ON_BLACK()) ; 

	if(result == Success)
		Message("[ OK ]", Display::Attribute(DisplayManager::FG_BLACK, DisplayManager::BG_GREEN)) ;
	else
		Message("[ FAILED ]", Display::Attribute(DisplayManager::FG_RED, DisplayManager::BG_WHITE)) ;
}

void Display::MoveCursor(int iOffSet)
{
	int iPos = DisplayManager::GetCurrentCursorPosition() + iOffSet ;
	
	if(iPos >= 0 && iPos < m_iMaxRows * m_iMaxCols)
	{
		DisplayManager::UpdateCursorPosition(iPos, true) ;
	}
}

void Display::SetCursor(int iCurPos, bool bUpdateCursorOnScreen)
{
	if(iCurPos >= 0 && iCurPos < m_iMaxRows * m_iMaxCols)
		DisplayManager::UpdateCursorPosition(iCurPos, bUpdateCursorOnScreen) ;
}

int Display::GetCursor()
{
	return DisplayManager::GetCurrentCursorPosition() ;
}

void Display::RawCharacter(char ch, const Display::Attribute& attr, bool bUpdateCursorOnScreen)
{
	DisplayManager::PutChar(DisplayManager::GetCurrentDisplayBytePosition() + 0, ch) ;
	DisplayManager::PutChar(DisplayManager::GetCurrentDisplayBytePosition() + 1, attr) ;
	DisplayManager::UpdateCursorPosition(DisplayManager::GetCurrentCursorPosition() + 1, bUpdateCursorOnScreen) ;
}

void Display::ShowProgress(const char* msg, int startCur, unsigned progNum)
{
	int c = GetCursor() ;
	
	while(c > startCur)
	{
		MoveCursor(-1) ;
		c-- ;
	}

	ClearLine(Display::START_CURSOR_POS) ;

	Number(msg, progNum) ;
}

const Display::Attribute& Display::WHITE_ON_BLACK()
{
	static const Display::Attribute mAttr(DisplayManager::FG_WHITE, DisplayManager::BG_BLACK) ;

	return mAttr ;
}

void Display::SetMouseCursorPos(int iPos)
{
	if(iPos < 0 || (iPos >= m_iMaxRows * m_iMaxCols))
		return ;
	DisplayManager::GetMouseCursor().SetCurPos(iPos) ;
}

int Display::GetMouseCursorPos()
{
	return DisplayManager::GetMouseCursor().GetCurPos() ;
}
