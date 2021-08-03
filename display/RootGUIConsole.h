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

#include <RootConsole.h>
#include <TextWriter.h>
#include <FrameBuffer.h>
#include <Viewport.h>
#include <BaseFrame.h>

class RootGUIConsole : public RootConsole {
private:
  static RootGUIConsole* _instance;
  static void Create();
  RootGUIConsole(const upanui::FrameBuffer& frameBuffer, const upanui::Viewport& viewport);

public:
  static RootGUIConsole& Instance();

  void gotoCursor() override {}
  void putChar(int iPos, byte ch, const upanui::CharStyle& attr) override;
  void scrollDown() override;

  void resetFrameBuffer(uint32_t frameBufferAddress);
  void setFontContext(upanui::usfn::Context*);
  upanui::BaseFrame& frame() {
    return _frame;
  }

private:
  upanui::TextWriter _textWriter;
  upanui::BaseFrame _frame;

  friend class RootConsole;
  friend class MemManager;
};