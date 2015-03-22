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
#ifndef _DISPLAY_MANAGER_H_
#define _DISPLAY_MANAGER_H_

#define DisplayManager_SUCCESS 0
#define DisplayManager_FAILURE 1

#define CURSOR_HIEGHT 16
#define CRT_INDEX_REG 0x03D4
#define CRT_DATA_REG 0x03D5

# include <Global.h>
# include <ctype.h>

class DisplayManager ;

class Cursor
{
	public:
		Cursor(const int& ccp, const int& cdp) : m_iCurrentCursorPosition(ccp), m_iCurrentDisplayBytePosition(cdp) { }

		inline void SetCurPos(int iCurPos) { m_iCurrentCursorPosition = iCurPos ; }
		inline void SetCurBytePos(int iCurBytePPos) { m_iCurrentDisplayBytePosition = iCurBytePPos ; }

		inline int GetCurPos() const { return m_iCurrentCursorPosition ; }
		inline int GetCurBytePos() const { return m_iCurrentDisplayBytePosition ; }

	private:
		int m_iCurrentCursorPosition ;
		int m_iCurrentDisplayBytePosition ;
} ;

class MouseCursor
{
	public:
		MouseCursor(const int& ccp) ;

		void SetCurPos(int iPos) ;
		inline int GetCurPos() const { return m_iCurrentCursorPosition ; }

		inline void SetOrigAttr(byte attr) { m_bOrigAttr = attr ; }
		inline byte GetOrigAttr() const { return m_bOrigAttr ; }

		inline void SetCursorAttr(byte attr) { m_bCursorAttr = attr ; }
		inline byte GetCursorAttr() { return m_bCursorAttr ; }

	private:
		byte m_bOrigAttr ;
		byte m_bCursorAttr ;
		int m_iCurrentCursorPosition ;
} ;

class DisplayBuffer
{
	private:
		Cursor m_mCursor ;
		unsigned m_uiDisplayMemRawAddress ;
	
	private:
		void Initialize(unsigned uiDisplayMemAddr) ;
		inline Cursor& GetCursor() { return m_mCursor ; }
		inline const Cursor& GetCursor() const { return m_mCursor ; }

	public:
		DisplayBuffer() ; 
		inline unsigned GetDisplayMemAddr() { return m_uiDisplayMemRawAddress ; }
		inline unsigned GetDisplayMemAddr() const { return m_uiDisplayMemRawAddress ; }

	friend class DisplayManager ;
} ;

class DisplayManager
{
	// Only a Display Object can use DisplayManager
	private:
		static const int NO_ROWS = 25 ;
		static const int NO_COLUMNS = 80 ;
		static const int NO_BYTES_PER_CHARACTER = 2 ;
		
	private:
		DisplayManager() ;
		
		static void InitCursor() ;

		static DisplayBuffer& GetDisplayBuffer(int pid) ;
		static DisplayBuffer& GetDisplayBuffer() ;
		static byte* GetDisplayMemAddress(int pid) ;

		static void PutChar(int iPos, byte ch) ;
		static byte GetChar(int iPos) ;

		static int GetCurrentCursorPosition() ;
		static int GetCurrentDisplayBytePosition() ;
		static void SetCurrentCursorPosition(int iPos) ;
		static void SetCurrentDisplayBytePosition(int iPos) ;

		static void UpdateCursorPosition(int iCursorPos, bool bUpdateCursorOnScreen) ;
		static void Goto(int x, int y) ;

		static DisplayBuffer& KernelDisplayBuffer() ;

		static MouseCursor& GetMouseCursor() ;
		static byte CheckMouseCursor(int iPos, byte ch) ;

	public:
		enum FGColor
		{
			FG_BLACK,
			FG_BLUE,
			FG_GREEN,
			FG_CYAN,
			FG_RED,
			FG_MAGENTA,
			FG_BROWN,
			FG_WHITE,
			FG_DARK_GRAY,
			FG_BRIGHT_BLUE,
			FG_BRIGHT_GREEN,
			FG_BRIGHT_CYAN,
			FG_PINK,
			FG_BRIGHT_MAGENTA,
			FG_YELLOW,
			FG_BRIGHT_WHITE,
		} ;

		enum BGColor
		{
			BG_BLACK = 0x00,
			BG_BLUE  = 0x10,
			BG_GREEN = 0x20,
			BG_CYAN  = 0x30,
			BG_RED   = 0x40,
			BG_MAGENTA = 0x50,
			BG_BROWN = 0x60,
			BG_WHITE = 0x70,
		} ;

		enum Blink
		{
			NO_BLINK = 0x00,
			BLINK = 0x80,
		} ;

	public:
		static void Initialize() ;
		static void InitializeDisplayBuffer(DisplayBuffer& rDisplayBuffer, unsigned uiDisplayMemAddr) ;

		static void RefreshScreen() ;
		static void SetupPageTableForDisplayBuffer(int iProcessGroupID, unsigned uiPDEAddress) ;

	friend class Display ;
} ;

#endif
