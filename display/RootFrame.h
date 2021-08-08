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

#include <BaseFrame.h>
#include <atomicop.h>

class RootFrame : public upanui::BaseFrame {
public:
  RootFrame(const upanui::FrameBuffer& frameBuffer, const upanui::Viewport& viewport) :
    BaseFrame(frameBuffer, viewport), _isDirty(false) {
  }

  void resetFrameBufferAddress(uint32_t* frameAddr) {
    const_cast<upanui::FrameBuffer&>(frameBuffer()).resetFrameBufferAddress(frameAddr);
    touch();
  }

  void touch() override {
    _isDirty.set(true);
  }

  bool isDirty() {
    return _isDirty.get();
  }

  void clean() {
    _isDirty.set(false);
  }

private:
  upan::atomic::integral<bool> _isDirty;
};