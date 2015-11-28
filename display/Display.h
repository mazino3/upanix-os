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
#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <Global.h>
#include <DisplayManager.h>

class Display
{
	public:
		class Attribute
		{
			public:
				Attribute() ;
				Attribute(const DisplayManager::FGColor& fgColor, const DisplayManager::BGColor& bgColor) ;
				Attribute(const DisplayManager::Blink& blink, const DisplayManager::FGColor& fgColor, const DisplayManager::BGColor& bgColor) ;
				Attribute(const byte& rawAttr) ;

				void SetBlink(const DisplayManager::Blink& blink) ;
				void SetFGColor(const DisplayManager::FGColor& fgColor) ;
				void SetBGColor(const DisplayManager::BGColor& bgColor) ;

				inline DisplayManager::Blink GetBlink() { return m_blink ; }
				inline DisplayManager::FGColor GetFGColor() { return m_fgColor ; }
				inline DisplayManager::BGColor GetBGColor() { return m_bgColor ; }

				inline byte GetAttrVal() { return m_Attr ; }
				inline byte GetAttrVal() const { return m_Attr ; }

				operator byte() const { return GetAttrVal() ; } 
				operator byte() { return GetAttrVal() ; } 

			private:
				DisplayManager::Blink m_blink ;
				DisplayManager::FGColor m_fgColor ;
				DisplayManager::BGColor m_bgColor ;
				byte m_Attr ;

				void UpdateAttrVal() ;
		} ;
		
		static const int START_CURSOR_POS = -1 ;

		Display() ;
		Display(const int& nMaxRows, const int& nMaxCols) ;

		int		ClearScreen() ;
		int		Message(const __volatile__ char* message, const Display::Attribute& attr = WHITE_ON_BLACK()) ;
		int		Message(const __volatile__ char* message, const byte& rawAttr) ;
		int		nMessage(const __volatile__ char* message, int len, const Display::Attribute& attr = WHITE_ON_BLACK()) ;

		void 	Address(const char* message, unsigned int address) ;
		void	Number(const char *message, DWORD dwNumber) ;
		void	DDNumberInHex(const char *message, DDWORD ddNumber) ;
		void	DDNumberInDec(const char *message, DDWORD ddNumber) ;
		void	Character(char ch, const Display::Attribute& attr = WHITE_ON_BLACK()) ;
		int		NextLine() ;
		void	LoadMessage(const char* loadMessage, Result result);
		void	ScrollDown() ;
		void	ClearLine(int iStartPos) ;
		void	MoveCursor(int iOffSet) ; 
		void	RawCharacter(char ch, const Display::Attribute& colorAttribute, bool bUpdateCursorOnScreen) ;
		void	SetCursor(int iCurPos, bool bUpdateCursorOnScreen) ;
		int		GetCursor() ;
		void	ShowProgress(const char* msg, int startCur, unsigned progNum) ;
		void	SetMouseCursorPos(int iPos) ;
		int		GetMouseCursorPos() ;
		int		GetMaxRows() const { return m_iMaxRows ; }
		int		GetMaxCols() const { return m_iMaxCols ; }

		static const Attribute& WHITE_ON_BLACK() ;

	private:
		const int m_iMaxRows ;
		const int m_iMaxCols ;
} ;

#endif
