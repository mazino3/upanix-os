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

#include <Display.h>
#include <KernelUtil.h>
#include <Atomic.h>

class GraphicsConsole : public Display, KernelUtil::TimerTask
{
private:
  GraphicsConsole(unsigned rows, unsigned columns);
  void GotoCursor() override;
  void DirectPutChar(int iPos, byte ch, byte attr) override;
  void DoScrollDown() override;
  void PutCursor(int pos, bool show);
  bool TimerTrigger() override;
  void StartCursorBlink() override;

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
  int _cursorPos;
  bool _cursorEnabled;
  Mutex _cursorMutex;
};