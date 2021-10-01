/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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

#include <RootConsole.h>
#include <TextWriter.h>
#include <FrameBuffer.h>
#include <Viewport.h>
#include <BaseFrame.h>
#include <timer_thread.h>
#include <RootFrame.h>

class RootGUIConsole : public RootConsole {
private:
  static RootGUIConsole* _instance;
  static void Create();
  RootGUIConsole(const upanui::FrameBuffer& frameBuffer, const upanui::Viewport& viewport);

public:
  static RootGUIConsole& Instance();

  void resetFrameBuffer(uint32_t frameBufferAddress);
  void setFontContext(upanui::usfn::Context*);
  RootFrame& frame() {
    return _frame;
  }

private:
  void gotoCursor() override;
  void putChar(int iPos, byte ch, const upanui::CharStyle& attr) override;
  void scrollDown() override;
  void putCursor(bool show);

  class CursorBlink : public upan::timer_thread {
  public:
    explicit CursorBlink(RootGUIConsole& console);
    void on_timer_trigger() override;
    RootGUIConsole& _console;
    bool _showCursorToggle;
  };
  void StartCursorBlink() override;

private:
  upanui::TextWriter _textWriter;
  RootFrame _frame;
  int _cursorPos;
  bool _cursorEnabled;
  upan::mutex _cursorMutex;

  friend class RootConsole;
  friend class MemManager;
};