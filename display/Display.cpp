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
#include <Display.h>
#include <PortCom.h>
#include <MemConstants.h>
#include <ProcessGroup.h>
#include <ProcessManager.h>
#include <DMM.h>
#include <Display.h>
#include <KernelComponents.h>
#include <GraphicsVideo.h>

#define VIDEO_BUFFER_ADDRESS			0xB8000

DisplayBuffer::DisplayBuffer(byte* buffer, unsigned rows, unsigned columns, bool isKernel)
  : _cursor(0, 0), 
    _buffer(buffer), 
    _bufSize(rows * columns * Display::NO_BYTES_PER_CHARACTER), 
    _isKerel(isKernel)
{
	for(unsigned i = 0; i < _bufSize; i += Display::NO_BYTES_PER_CHARACTER)
	{
		_buffer[i] = ' ';
		_buffer[i + 1] = Display::WHITE_ON_BLACK();
	}
}

DisplayBuffer::~DisplayBuffer()
{
  if(!_isKerel)
    delete []_buffer;
}

Display::Attribute::Attribute() : 
	m_blink(Display::NO_BLINK), 
	m_fgColor(Display::FG_WHITE),
	m_bgColor(Display::BG_BLACK)
{
	UpdateAttrVal();
}

Display::Attribute::Attribute(const Display::FGColor& fgColor, const Display::BGColor& bgColor) :
	m_blink(Display::NO_BLINK),
	m_fgColor(fgColor),
	m_bgColor(bgColor)
{
	UpdateAttrVal();
}

Display::Attribute::Attribute(const Display::Blink& blink, const Display::FGColor& fgColor, const Display::BGColor& bgColor) :
	m_blink(blink),
	m_fgColor(fgColor),
	m_bgColor(bgColor)
{
	UpdateAttrVal();
}

Display::Attribute::Attribute(const byte& rawAttr)
{
	m_blink = static_cast<Display::Blink>(rawAttr & 0x80);
	m_fgColor = static_cast<Display::FGColor>(rawAttr & Display::FG_BRIGHT_WHITE);
	m_bgColor = static_cast<Display::BGColor>(rawAttr & Display::BG_WHITE);
  UpdateAttrVal();
}

void Display::Attribute::SetBlink(const Display::Blink& blink)
{
	m_blink = blink;
	UpdateAttrVal();
}

void Display::Attribute::SetFGColor(const Display::FGColor& fgColor)
{
	m_fgColor = fgColor;
	UpdateAttrVal();
}

void Display::Attribute::SetBGColor(const Display::BGColor& bgColor)
{
	m_bgColor = bgColor;
	UpdateAttrVal();
}

void Display::Attribute::UpdateAttrVal()
{
	m_Attr = m_blink | m_fgColor | m_bgColor;
}

void Display::Create()
{
	static bool bDone = false;
  if(bDone)
  {
    KC::MDisplay().Message("\n Display is already initialized!", Display::WHITE_ON_BLACK());
    return;
  }
  auto f = MultiBoot::Instance().VideoFrameBufferInfo();
  if(f)
  {
    static GraphicsTextConsole gc(f->framebuffer_height / 16, f->framebuffer_width / 8); 
    KC::SetDisplay(gc);
  }
  else
  {
    static VGATextConsole vc;
    KC::SetDisplay(vc);
  }
  KC::MDisplay().UpdateCursorPosition(0, true);
  KC::MDisplay().LoadMessage("Video Initialization", Success);
  bDone = true;
}

Display::Display(unsigned rows, unsigned height) 
  : _maxRows(rows), _maxColumns(height), 
    _kernelBuffer((byte*)(MEM_GRAPHICS_TEXT_BUFFER_START - GLOBAL_DATA_SEGMENT_BASE), rows, height, true)
{
}

void Display::ClearScreen()
{
	static const unsigned NO_OF_DISPLAY_BYTES = _maxRows * _maxColumns * NO_BYTES_PER_CHARACTER;
	for(unsigned i = 0; i < NO_OF_DISPLAY_BYTES; i += 2)
    PutChar(i, ' ', WHITE_ON_BLACK());
  UpdateCursorPosition(0, true);
}

void Display::UpdateCursorPosition(int iCursorPos, bool bUpdateCursorOnScreen)
{
	DisplayBuffer& mDisplayBuffer = GetDisplayBuffer();
	
	mDisplayBuffer.GetCursor().SetCurPos(iCursorPos);
	mDisplayBuffer.GetCursor().SetCurBytePos(iCursorPos * 2);

	if(bUpdateCursorOnScreen && ( IS_KERNEL() || IS_FG_PROCESS_GROUP() ))
	{
		int x = mDisplayBuffer.GetCursor().GetCurPos() % _maxColumns;
		int y = mDisplayBuffer.GetCursor().GetCurPos() / _maxColumns;

		Goto(x, y);
	}
}

void Display::Address(const char *message, unsigned int address)
{
	__volatile__ char addressDigit[9];
	__volatile__ byte i;

	for(i = 0; i < 8; i++)
	{
		addressDigit[7 - i] = (char)((address >> 4*i) & 0x0F);

		if(addressDigit[7 - i] < 10)
			addressDigit[7 - i] += 0x30;
		else
			addressDigit[7 - i] += (0x41 - 0x0A);
	}

	addressDigit[8] = '\0';

	for(i = 0; i < 7; i++)
		if(addressDigit[i] != '0')
			break;

	unsigned j;
	for(j = 0; i < 9; i++, j++)
		addressDigit[j] = addressDigit[i];

	Message(message, WHITE_ON_BLACK());
	Message(addressDigit, WHITE_ON_BLACK());
}

void Display::Number(const char *message, DWORD dwNumber)
{
	__volatile__ char szNumber[21];
	__volatile__ byte i;

	for(i = 0; i < 20; i++)
	{
		szNumber[i] = (dwNumber % 10) + 0x30;
		dwNumber = dwNumber / 10;
		if(!dwNumber)
			break;
	}

	i++;

	szNumber[i] = '\0';

	strreverse((char*)szNumber);

	Message(message, WHITE_ON_BLACK());
	Message(szNumber, WHITE_ON_BLACK());
}

void Display::DDNumberInHex(const char *message, DDWORD ddNumber)
{
	__volatile__ char addressDigit[17];
	__volatile__ byte i;

	for(i = 0; i < 16; i++)
	{
		addressDigit[15 - i] = (char)((ddNumber >> 4*i) & 0x0F);

		if(addressDigit[15 - i] < 10)
			addressDigit[15 - i] += 0x30;
		else
			addressDigit[15 - i] += (0x41 - 0x0A);
	}

	addressDigit[16] = '\0';

	for(i = 0; i < 15; i++)
		if(addressDigit[i] != '0')
			break;

	unsigned j;
	for(j = 0; i < 17; i++, j++)
		addressDigit[j] = addressDigit[i];

	Message(message, WHITE_ON_BLACK());
	Message(addressDigit, WHITE_ON_BLACK());
}

void Display::DDNumberInDec(const char *message, DDWORD ddNumber)
{
	__volatile__ char addressDigit[17];
	__volatile__ byte i;

	for(i = 0; i < 16; i++)
	{
		addressDigit[15 - i] = (char)((ddNumber >> 4*i) & 0x0F);

		if(addressDigit[15 - i] < 10)
			addressDigit[15 - i] += 0x30;
		else
			addressDigit[15 - i] += (0x41 - 0x0A);
	}

	addressDigit[16] = '\0';

	for(i = 0; i < 15; i++)
		if(addressDigit[i] != '0')
			break;

	unsigned j;
	for(j = 0; i < 17; i++, j++)
		addressDigit[j] = addressDigit[i];

	Message(message, WHITE_ON_BLACK());
	Message(addressDigit, WHITE_ON_BLACK());
}

DisplayBuffer& Display::GetDisplayBuffer(int pid)
{
	if(IS_KERNEL())
		return _kernelBuffer;
	return ProcessManager::Instance().GetAddressSpace(pid)._processGroup->GetDisplayBuffer();
}

DisplayBuffer& Display::GetDisplayBuffer()
{
	return GetDisplayBuffer(ProcessManager::GetCurrentProcessID());
}

int Display::GetCurrentCursorPosition()
{
	return GetDisplayBuffer().GetCursor().GetCurPos();
}

int Display::GetCurrentDisplayBytePosition()
{
	return GetDisplayBuffer().GetCursor().GetCurBytePos();
}

void Display::SetCurrentCursorPosition(int iPos)
{
	GetDisplayBuffer().GetCursor().SetCurPos(iPos);
}

void Display::SetCurrentDisplayBytePosition(int iPos)
{
	GetDisplayBuffer().GetCursor().SetCurBytePos(iPos);
}

void Display::NextLine()
{
	int curLine = (GetCurrentCursorPosition() / _maxColumns) + 1;

	if(curLine < (int)_maxColumns)
		UpdateCursorPosition(curLine * _maxColumns, true);
	else
		SetCurrentCursorPosition(_maxRows * _maxColumns);

  ScrollDown();
}

void Display::ClearLine(int iStartPos)
{
	if(iStartPos == START_CURSOR_POS)
		iStartPos = GetCurrentCursorPosition();
	else if(!(iStartPos >= 0 && iStartPos < (int)_maxColumns))
		iStartPos = GetCurrentCursorPosition();

	for(int i = iStartPos * 2; (i % (_maxColumns * 2)) != 0; i += 2)
    PutChar(i, ' ', WHITE_ON_BLACK());
}

void Display::RawCharacter(char ch, const Attribute& attr, bool bUpdateCursorOnScreen)
{
  PutChar(GetCurrentDisplayBytePosition(), ch, attr);
  UpdateCursorPosition(GetCurrentCursorPosition() + 1, bUpdateCursorOnScreen);
}

void Display::MoveCursor(int iOffSet)
{
	int iPos = GetCurrentCursorPosition() + iOffSet;
	
	if(iPos >= 0 && iPos < (int)(_maxColumns * _maxRows))
		UpdateCursorPosition(iPos, true);
}

void Display::SetCursor(int iCurPos, bool bUpdateCursorOnScreen)
{
	if(iCurPos >= 0 && iCurPos < (int)(_maxRows * _maxColumns))
		UpdateCursorPosition(iCurPos, bUpdateCursorOnScreen);
}

void Display::Character(char ch, const Attribute& attr)
{
	if(ch == '\n')
	{
		NextLine();
	}
	else if(ch == '\t')
	{
		int i;
		for(i = 0; i < 4; i++)
			Character(' ', attr);
	}
	else
	{
    PutChar(GetCurrentDisplayBytePosition(), ch, attr);
    UpdateCursorPosition(GetCurrentCursorPosition() + 1, true);

		ScrollDown();
	}
}

void Display::Message(const __volatile__ char* message, const Attribute& attr)
{
	for(int i = 0; message[i] != '\0'; i++)
		Character(message[i], attr);
}

void Display::Message(const __volatile__ char* message, const byte& rawAttr)
{
	Attribute attr(rawAttr);
	for(int i = 0; message[i] != '\0'; i++)
		Character(message[i], attr);
}

void Display::nMessage(const __volatile__ char* message, int len, const Attribute& attr)
{
	for(int i = 0; i < len; i++)
		Character(message[i], attr);
}

void Display::LoadMessage(const char* loadMessage, ReturnCode result)
{
	byte width = 50;
	unsigned int i;
	char spaces[50] = "\0";
	
	for(i = 0; loadMessage[i] != '\0'; i++);

	width = width - i;
	
	for(i = 0; i < width; i++)
			spaces[i] = ' ';
	spaces[i] = '\0';
	
	Message("\n ", WHITE_ON_BLACK());
	Message(loadMessage, WHITE_ON_BLACK());
	Message(spaces, WHITE_ON_BLACK()); 

	if(result == Success)
		Message("[ OK ]", Attribute(Display::FG_BLACK, Display::BG_GREEN));
	else
		Message("[ FAILED ]", Attribute(Display::FG_RED, Display::BG_WHITE));
}

void Display::ShowProgress(const char* msg, int startCur, unsigned progNum)
{
  for(int c = GetCurrentCursorPosition(); c > startCur; --c)
		MoveCursor(-1);
	ClearLine(START_CURSOR_POS);
	Number(msg, progNum);
}

byte Display::GetChar(int iPos)
{
  return GetDisplayBuffer().GetChar(iPos);
}

void Display::PutCharOnBuffer(int iPos, byte ch, byte attr)
{
  auto& db = GetDisplayBuffer();
  db.PutChar(iPos, ch);
  db.PutChar(iPos + 1, attr);
}

void Display::PutChar(int iPos, byte ch, byte attr)
{
  PutCharOnBuffer(iPos, ch, attr);
	if(IS_KERNEL() || IS_FG_PROCESS_GROUP())
    DirectPutChar(iPos, ch, attr);
}

void Display::ScrollDown()
{
	if(!(GetCurrentCursorPosition() >= (int)(_maxRows * _maxColumns)))
		return;

  static const unsigned NO_OF_DISPLAY_BYTES = (_maxRows - 1) * _maxColumns * NO_BYTES_PER_CHARACTER;
  static const unsigned OFFSET = _maxColumns * NO_BYTES_PER_CHARACTER;

  byte* buffer = GetDisplayBuffer().GetBuffer();
  memcpy(buffer, buffer + OFFSET, NO_OF_DISPLAY_BYTES);
	for(unsigned i = NO_OF_DISPLAY_BYTES; i < NO_OF_DISPLAY_BYTES + _maxColumns * NO_BYTES_PER_CHARACTER; i += 2)
  {
    buffer[i] = ' '; 
    buffer[i + 1] = WHITE_ON_BLACK();
  }

  if(IS_KERNEL() || IS_FG_PROCESS_GROUP())
    DoScrollDown();

  UpdateCursorPosition(NO_OF_DISPLAY_BYTES / 2, true);
}

VGATextConsole::VGATextConsole() : Display(25, 80)
{
  InitCursor();
}

void VGATextConsole::InitCursor()
{
  // Get Cursor Size Start
  PortCom_SendByte(CRT_INDEX_REG, 0x0A);
  byte bCurStart = PortCom_ReceiveByte(CRT_DATA_REG);
  // Get Cursor Size End
  PortCom_SendByte(CRT_INDEX_REG, 0x0A);
  byte bCurEnd = PortCom_ReceiveByte(CRT_DATA_REG);

  bCurStart = (bCurStart & 0xC0 ) | (CURSOR_HIEGHT - 3);
  bCurEnd = (bCurEnd & 0xE0 ) | (CURSOR_HIEGHT - 2);

  // Set Cursor Size Start 
  PortCom_SendByte(CRT_INDEX_REG, 0x0A);
  PortCom_SendByte(CRT_DATA_REG, bCurStart);
  // Set Cursor Size End
  PortCom_SendByte(CRT_INDEX_REG, 0x0B);
  PortCom_SendByte(CRT_DATA_REG, bCurEnd);
}

void VGATextConsole::Goto(int x, int y)
{
  int iPos = x + y * _maxColumns;

  PortCom_SendByte(CRT_INDEX_REG, 15);
  PortCom_SendByte(CRT_DATA_REG, iPos & 0xFF);

  PortCom_SendByte(CRT_INDEX_REG, 14);
  PortCom_SendByte(CRT_DATA_REG, (iPos >> 8) & 0xFF);
}

void VGATextConsole::DoScrollDown()
{
  static const unsigned NO_OF_DISPLAY_BYTES = (_maxRows - 1) * _maxColumns * NO_BYTES_PER_CHARACTER;
  static const unsigned OFFSET = _maxColumns * NO_BYTES_PER_CHARACTER;

  static byte* vga_frame_buffer = (byte*)(VIDEO_BUFFER_ADDRESS - GLOBAL_DATA_SEGMENT_BASE);
  memcpy(vga_frame_buffer, vga_frame_buffer + OFFSET, NO_OF_DISPLAY_BYTES);
	for(unsigned i = NO_OF_DISPLAY_BYTES; i < NO_OF_DISPLAY_BYTES + _maxColumns * NO_BYTES_PER_CHARACTER; i += 2)
  {
    vga_frame_buffer[i] = ' '; 
    vga_frame_buffer[i + 1] = WHITE_ON_BLACK();
  }
}

GraphicsTextConsole::GraphicsTextConsole(unsigned rows, unsigned columns) : Display(rows, columns)
{
  GraphicsVideo::Create();
}

void GraphicsTextConsole::Goto(int x, int y)
{
  //TODO
}

void VGATextConsole::DirectPutChar(int iPos, byte ch, byte attr)
{
  static byte* vga_frame_buffer = (byte*)(VIDEO_BUFFER_ADDRESS - GLOBAL_DATA_SEGMENT_BASE);
  vga_frame_buffer[iPos] = ch;
  vga_frame_buffer[iPos + 1] = attr;
}

void GraphicsTextConsole::DirectPutChar(int iPos, byte ch, byte attr)
{
  static const unsigned GRAPHICS_COLOR[] = {
    0x000000, //FG_BLACK
    0x0000FF, //FG_BLUE
    0x00FF00, //FG_GREEN
    0x00FFFF, //FG_CYAN
    0xFF0000, //FG_RED
    0xFF00FF, //FG_MAGENTA
    0xA52A2A, //FG_BROWN
    0xFFFFFF, //FG_WHITE
    0x404040, //FG_DARK_GRAY
    0x1A1AFF, //FG_BRIGHT_BLUE
    0x1AFF1A, //FG_BRIGHT_GREEN
    0x1AFFFF, //FG_BRIGHT_CYAN
    0xFFC0CB, //FG_PINK
    0xFF1AFF, //FG_BRIGHT_MAGENTA
    0xFFFF00, //FG_YELLOW
    0xFFFFFF, //FG_BRIGHT_WHITE
  };
  const int curPos = iPos / NO_BYTES_PER_CHARACTER;
  const unsigned x = (curPos % _maxColumns);
	const unsigned y = (curPos / _maxColumns);

  GraphicsVideo::Instance()->DrawChar(ch, x, y, 
    GRAPHICS_COLOR[attr & FG_BRIGHT_WHITE], 
    GRAPHICS_COLOR[(attr & BG_WHITE) >> 4]);
}

void GraphicsTextConsole::DoScrollDown()
{
  GraphicsVideo::Instance()->ScrollDown();
}

void Display::RefreshScreen()
{
	ProcessGroup* pFGGroup = ProcessGroup::GetFGProcessGroup();
  if(!pFGGroup)
    return;

  DisplayBuffer& db = pFGGroup->GetDisplayBuffer();

	const unsigned noOfDisplayBytes = TextBufferSize();
	byte* buffer = db.GetBuffer();

	for(unsigned i = 0; i < noOfDisplayBytes; i += 2)
    DirectPutChar(i, buffer[i], buffer[i+1]);

	int x = db.GetCursor().GetCurPos() % _maxColumns;
	int y = db.GetCursor().GetCurPos() / _maxColumns;

	Goto(x, y);
}

DisplayBuffer& Display::CreateDisplayBuffer()
{
  return *new DisplayBuffer(new byte[ TextBufferSize() ], _maxRows, _maxColumns, false);
}

const Display::Attribute& Display::WHITE_ON_BLACK()
{
	static const Attribute mAttr(FG_WHITE, BG_BLACK);
	return mAttr;
}

#undef VIDOMEM
