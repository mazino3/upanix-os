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
#pragma once

# include <Global.h>
# include <ctype.h>
# include <cdisplay.h>
# include <DisplayConstants.h>
# include <ColorPalettes.h>

class Display;

class Cursor
{
	public:
		Cursor(int cursorPos) : _cursorPos(cursorPos) { }

		inline void SetCurPos(int cursorPos) {
      _cursorPos = cursorPos;
		}

		inline int GetCurPos() const { return _cursorPos; }
		inline int GetCurBytePos() const { return _cursorPos * DisplayConstants::NO_BYTES_PER_CHARACTER; }

	private:
		int _cursorPos;
};

class DisplayBuffer
{
  public:
    ~DisplayBuffer();
    byte* GetBuffer() { return _buffer; }
	private:
		DisplayBuffer(byte* buffer, unsigned rows, unsigned height, bool isKernel);

    bool PutChar(int pos, byte val) {
      bool changed = _buffer[pos] != val;
      if (changed) {
        _buffer[pos] = val;
      }
      return changed;
    }
    byte GetChar(int pos) { return _buffer[pos]; }
		inline Cursor& GetCursor() { return _cursor; }
		inline const Cursor& GetCursor() const { return _cursor; }

		Cursor         _cursor;
    byte*          _buffer;
    const unsigned _bufSize;
    const bool     _isKerel;

	friend class Display;
};

class Display
{
  public:
		class Attribute
		{
			public:
				Attribute();
				Attribute(const ColorPalettes::CP16::FGColor& fgColor, const ColorPalettes::CP16::BGColor& bgColor);
				Attribute(const DisplayConstants::Blink& blink, const ColorPalettes::CP16::FGColor& fgColor, const ColorPalettes::CP16::BGColor& bgColor);
				Attribute(const byte& rawAttr);

				void SetBlink(const DisplayConstants::Blink& blink);
				void SetFGColor(const ColorPalettes::CP16::FGColor& fgColor);
				void SetBGColor(const ColorPalettes::CP16::BGColor& bgColor);

				inline DisplayConstants::Blink GetBlink() { return m_blink; }
				inline ColorPalettes::CP16::FGColor GetFGColor() { return m_fgColor; }
				inline ColorPalettes::CP16::BGColor GetBGColor() { return m_bgColor; }

				inline byte GetAttrVal() { return m_Attr; }
				inline byte GetAttrVal() const { return m_Attr; }

				operator byte() const { return GetAttrVal(); } 
				operator byte() { return GetAttrVal(); } 

			private:
        DisplayConstants::Blink m_blink;
        ColorPalettes::CP16::FGColor m_fgColor;
        ColorPalettes::CP16::BGColor m_bgColor;
				byte m_Attr;

				void UpdateAttrVal();
		};

    static void CreateDefault();
    static void CreateGraphicsConsole();

    DisplayBuffer& CreateDisplayBuffer();
    unsigned TextBufferSize() const { return _maxRows * _maxColumns * DisplayConstants::NO_BYTES_PER_CHARACTER; }
    unsigned MaxRows() const { return _maxRows; }
    unsigned MaxColumns() const { return _maxColumns; }
    void ClearScreen();
		void UpdateCursorPosition(int iCursorPos, bool bUpdateCursorOnScreen);
    void NextLine();
    void Message(const __volatile__ char* message, const Attribute& attr);
    void Message(const __volatile__ char* message, const byte& rawAttr);
    void nMessage(const __volatile__ char* message, int len, const Attribute& attr);
    void Character(char ch, const Attribute& attr);
    void RawCharacter(byte ch, const Attribute& attr, bool bUpdateCursorOnScreen);
    void RawCharacterArea(const MChar* src, uint32_t rows, uint32_t cols, int curPos);
    void MoveCursor(int iOffSet);
    void SetCursor(int iCurPos, bool bUpdateCursorOnScreen);
		void LoadMessage(const char* loadMessage, ReturnCode result);
		void ShowProgress(const char* msg, int startCur, unsigned progressPercent);
    void ClearLine(int iStartPos);
		void RefreshScreen();
		int GetCurrentCursorPosition();
    void ScrollDown();

		static const int START_CURSOR_POS = -1;
		static const Attribute& WHITE_ON_BLACK();

		DisplayBuffer& GetDisplayBuffer();
    virtual void StartCursorBlink() {
    }

protected:
		Display(unsigned rows, unsigned columns);
		
    bool PutCharOnBuffer(int iPos, byte ch, byte attr);
    void PutChar(int iPos, byte ch, byte attr);
    virtual void DirectPutChar(int iPos, byte ch, byte attr) = 0;
		DisplayBuffer& GetDisplayBuffer(int pid);

		virtual void GotoCursor() = 0;
    virtual void DoScrollDown() = 0;

		//byte* GetDisplayMemAddress(int pid);
		byte GetChar(int iPos);

		int GetCurrentDisplayBytePosition();

  protected:
    const unsigned _maxRows;
    const unsigned _maxColumns;
    DisplayBuffer  _kernelBuffer;
};
