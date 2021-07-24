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

#include <StreamBufferDescriptor.h>
#include <fs.h>
#include <Display.h>

StreamBufferDescriptor::StreamBufferDescriptor(int pid, int id, uint32_t bufSize)
  : IODescriptor(pid, id, O_APPEND), _bufSize(bufSize) {
  _buffer.reset(new byte[bufSize]);
}

int StreamBufferDescriptor::read(char* buffer, int len) {
  return 0;
}

int StreamBufferDescriptor::write(const char* buffer, int len) {
  KC::MDisplay().nMessage(buffer, len, Display::WHITE_ON_BLACK());
  return len;
}