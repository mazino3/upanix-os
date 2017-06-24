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

#define CURSOR_HIEGHT 16
#define CRT_INDEX_REG 0x03D4
#define CRT_DATA_REG 0x03D5

# include <Global.h>
# include <ctype.h>

class Display;

class Cursor
{
	public:
		Cursor(const int& ccp, const int& cdp) : m_iCurrentCursorPosition(ccp), m_iCurrentDisplayBytePosition(cdp) { }

		inline void SetCurPos(int iCurPos) { m_iCurrentCursorPosition = iCurPos; }
		inline void SetCurBytePos(int iCurBytePPos) { m_iCurrentDisplayBytePosition = iCurBytePPos; }

		inline int GetCurPos() const { return m_iCurrentCursorPosition; }
		inline int GetCurBytePos() const { return m_iCurrentDisplayBytePosition; }

	private:
		int m_iCurrentCursorPosition;
		int m_iCurrentDisplayBytePosition;
};

class DisplayBuffer
{
  public:
    ~DisplayBuffer();
	private:
		DisplayBuffer(byte* buffer, unsigned rows, unsigned height, bool isKernel);

    void PutChar(int pos, byte val) { _buffer[pos] = val; }
    byte GetChar(int pos) { return _buffer[pos]; }
    byte* GetBuffer() { return _buffer; }
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
		};

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
		};

		enum Blink
		{
			NO_BLINK = 0x00,
			BLINK = 0x80,
		};

		class Attribute
		{
			public:
				Attribute();
				Attribute(const FGColor& fgColor, const Display::BGColor& bgColor);
				Attribute(const Blink& blink, const Display::FGColor& fgColor, const BGColor& bgColor);
				Attribute(const byte& rawAttr);

				void SetBlink(const Display::Blink& blink);
				void SetFGColor(const Display::FGColor& fgColor);
				void SetBGColor(const Display::BGColor& bgColor);

				inline Blink GetBlink() { return m_blink; }
				inline FGColor GetFGColor() { return m_fgColor; }
				inline BGColor GetBGColor() { return m_bgColor; }

				inline byte GetAttrVal() { return m_Attr; }
				inline byte GetAttrVal() const { return m_Attr; }

				operator byte() const { return GetAttrVal(); } 
				operator byte() { return GetAttrVal(); } 

			private:
				Display::Blink m_blink;
				Display::FGColor m_fgColor;
				Display::BGColor m_bgColor;
				byte m_Attr;

				void UpdateAttrVal();
		};

    static const int NO_BYTES_PER_CHARACTER = 2;

    static void Create();
    DisplayBuffer& CreateDisplayBuffer();
    unsigned TextBufferSize() const { return _maxRows * _maxColumns * NO_BYTES_PER_CHARACTER; }
    unsigned MaxRows() const { return _maxRows; }
    unsigned MaxColumns() const { return _maxColumns; }
    void ClearScreen();
		void UpdateCursorPosition(int iCursorPos, bool bUpdateCursorOnScreen);
    void NextLine();
    void Message(const __volatile__ char* message, const Attribute& attr);
    void Message(const __volatile__ char* message, const byte& rawAttr);
    void nMessage(const __volatile__ char* message, int len, const Attribute& attr);
		void Address(const char* message, unsigned int address);
		void Number(const char *message, DWORD dwNumber);
		void DDNumberInHex(const char *message, DDWORD ddNumber);
		void DDNumberInDec(const char *message, DDWORD ddNumber);
    void Character(char ch, const Attribute& attr);
    void RawCharacter(char ch, const Attribute& attr, bool bUpdateCursorOnScreen);
    void MoveCursor(int iOffSet);
    void SetCursor(int iCurPos, bool bUpdateCursorOnScreen);
		void LoadMessage(const char* loadMessage, ReturnCode result);
		void ShowProgress(const char* msg, int startCur, unsigned progNum);
    void ClearLine(int iStartPos);
		void RefreshScreen();
		int GetCurrentCursorPosition();
    void ScrollDown();

    //TODO
    int GetMouseCursorPos() { return 0; }
    void SetMouseCursorPos(int pos) { }

		static const int START_CURSOR_POS = -1;
		static const Attribute& WHITE_ON_BLACK();

	protected:
		Display(unsigned rows, unsigned columns);
		
    void PutCharOnBuffer(int iPos, byte ch, byte attr);
    void PutChar(int iPos, byte ch, byte attr);
    virtual void DirectPutChar(int iPos, byte ch, byte attr) = 0;
		DisplayBuffer& GetDisplayBuffer(int pid);
		DisplayBuffer& GetDisplayBuffer();

		virtual void Goto(int x, int y) = 0;
    virtual void DoScrollDown() = 0;

		byte* GetDisplayMemAddress(int pid);
		byte GetChar(int iPos);

		int GetCurrentDisplayBytePosition();
		void SetCurrentCursorPosition(int iPos);
		void SetCurrentDisplayBytePosition(int iPos);

  protected:
    const unsigned _maxRows;
    const unsigned _maxColumns;
    DisplayBuffer  _kernelBuffer;
};

class VGATextConsole : public Display
{
  private:
    VGATextConsole();
		void InitCursor();
		virtual void Goto(int x, int y);
    virtual void DirectPutChar(int iPos, byte ch, byte attr);
    virtual void DoScrollDown();

    class VideoBuffer : public DisplayBuffer
    {
      public:
        VideoBuffer(unsigned uiDisplayMemAddr);
    };
  friend class Display;
};

class GraphicsTextConsole : public Display
{
  private:
    GraphicsTextConsole(unsigned rows, unsigned columns);
		virtual void Goto(int x, int y);
    virtual void DirectPutChar(int iPos, byte ch, byte attr);
    virtual void DoScrollDown();

    class VideoBuffer : public DisplayBuffer
    {
      public:
        VideoBuffer(unsigned uiDisplayMemAddr);

      private:
        const unsigned _width;
        const unsigned _height;
        const unsigned _pitch;
        const byte     _bpp;
        const byte     _bytesPerPixel;
    };
  friend class Display;
};

#endif
