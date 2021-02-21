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

#include <GraphicsConsole.h>
#include <GraphicsVideo.h>

GraphicsConsole::GraphicsConsole(unsigned rows, unsigned columns) : Display(rows, columns)
{
  GraphicsVideo::Create();
}

void GraphicsConsole::Goto(int x, int y)
{
  //TODO
}

void GraphicsConsole::DirectPutChar(int iPos, byte ch, byte attr)
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

void GraphicsConsole::DoScrollDown()
{
  GraphicsVideo::Instance()->ScrollDown();
}
