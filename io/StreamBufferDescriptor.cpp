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
#include <ProcessConstants.h>
#include <KernelComponents.h>

StreamBufferDescriptor::StreamBufferDescriptor(int pid, int id, uint32_t bufSize)
  : IODescriptor(pid, id, O_APPEND), _queue(bufSize) {
}

int StreamBufferDescriptor::read(char* buffer, int len) {
  return _queue.read(buffer, len);
}

int StreamBufferDescriptor::write(const char* buffer, int len) {
  if (getPid() == NO_PROCESS_ID) {
    KC::MConsole().nMessage(buffer, len, upanui::CharStyle::WHITE_ON_BLACK());
    return len;
  } else {
    return _queue.write(buffer, len);
    //KC::MConsole().nMessage(buffer, len, upanui::CharStyle::WHITE_ON_BLACK());
    //return len;
  }
}