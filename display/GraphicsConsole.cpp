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
#include <ColorPalettes.h>

GraphicsConsole::GraphicsConsole(unsigned rows, unsigned columns)
  : Display(rows, columns), upan::timer_thread(500), _cursorPos(0), _cursorEnabled(false) {
}

void GraphicsConsole::StartCursorBlink() {
  run();
  _cursorEnabled = true;
}

void GraphicsConsole::GotoCursor() {
  if (_cursorEnabled) {
    upan::mutex_guard g(_cursorMutex);
    int newCurPos = GetCurrentCursorPosition();
    if (newCurPos != _cursorPos) {
      //erase old cursor
      PutCursor(_cursorPos, false);
    }
    //draw new cursor
    PutCursor(newCurPos, true);
    _cursorPos = newCurPos;
  }
}

void GraphicsConsole::PutCursor(int pos, bool show) {
  if ((uint32_t)pos >= _maxRows * _maxColumns) {
    return;
  }

  const auto attr = GetChar(pos * DisplayConstants::NO_BYTES_PER_CHARACTER + 1);
  const auto color = show ? ColorPalettes::CP16::Get(attr & ColorPalettes::CP16::FG_WHITE)
      : ColorPalettes::CP16::Get((attr & ColorPalettes::CP16::BG_WHITE) >> 4);
  const auto x = (pos % _maxColumns);
  const auto y = (pos / _maxColumns);

  GraphicsVideo::Instance()->DrawCursor(x, y, color);
}

void GraphicsConsole::on_timer_trigger() {
  upan::mutex_guard g(_cursorMutex);
  static bool showCursor = false;
  PutCursor(_cursorPos, showCursor);
  showCursor = !showCursor;
}

void GraphicsConsole::DirectPutChar(int iPos, byte ch, byte attr)
{
  const int curPos = iPos / DisplayConstants::NO_BYTES_PER_CHARACTER;
  const unsigned x = (curPos % _maxColumns);
  const unsigned y = (curPos / _maxColumns);

  GraphicsVideo::Instance()->DrawChar(ch, x, y,
                                      ColorPalettes::CP16::Get(attr & ColorPalettes::CP16::FG_WHITE),
                                      ColorPalettes::CP16::Get((attr & ColorPalettes::CP16::BG_WHITE) >> 4));
}

void GraphicsConsole::DoScrollDown()
{
  GraphicsVideo::Instance()->ScrollDown();
}
