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

#include <VGAConsole.h>
#include <PortCom.h>

#define VIDEO_BUFFER_ADDRESS			0xB8000

VGAConsole::VGAConsole() : Display(25, 80)
{
  InitCursor();
}

void VGAConsole::InitCursor()
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

void VGAConsole::Goto(int x, int y)
{
  int iPos = x + y * _maxColumns;

  PortCom_SendByte(CRT_INDEX_REG, 15);
  PortCom_SendByte(CRT_DATA_REG, iPos & 0xFF);

  PortCom_SendByte(CRT_INDEX_REG, 14);
  PortCom_SendByte(CRT_DATA_REG, (iPos >> 8) & 0xFF);
}

void VGAConsole::DoScrollDown()
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

void VGAConsole::DirectPutChar(int iPos, byte ch, byte attr)
{
  static byte* vga_frame_buffer = (byte*)(VIDEO_BUFFER_ADDRESS - GLOBAL_DATA_SEGMENT_BASE);
  vga_frame_buffer[iPos] = ch;
  vga_frame_buffer[iPos + 1] = attr;
}


