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

#include <DefaultConsole.h>
#include <GraphicsVideo.h>
#include <ColorPalettes.h>

DefaultConsole::DefaultConsole(unsigned rows, unsigned columns) : Display(rows, columns) {
  GraphicsVideo::Create();
}

void DefaultConsole::DirectPutChar(int iPos, byte ch, byte attr)
{
  const int curPos = iPos / DisplayConstants::NO_BYTES_PER_CHARACTER;
  const unsigned x = (curPos % _maxColumns);
  const unsigned y = (curPos / _maxColumns);

  GraphicsVideo::Instance()->DrawChar(ch, x, y,
                                      ColorPalettes::CP16::Get(attr & ColorPalettes::CP16::FG_WHITE),
                                      ColorPalettes::CP16::Get((attr & ColorPalettes::CP16::BG_WHITE) >> 4));
}

void DefaultConsole::DoScrollDown()
{
  GraphicsVideo::Instance()->ScrollDown();
}
