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
#include <ProcessManager.h>

StreamBufferDescriptor::StreamBufferDescriptor(int pid, int id, uint32_t bufSize, uint32_t mode)
  : IODescriptor(pid, id, O_APPEND | mode), _queue(bufSize) {
}

int StreamBufferDescriptor::read(char* buffer, int len) {
  while(true) {
    {
      upan::mutex_guard g(_ioSync);
      if (!_queue.empty()) {
        return _queue.read(buffer, len);
      }
    }
    if (getMode() & O_RD_NONBLOCK) {
      return 0;
    }
    ProcessManager::Instance().WaitOnIODescriptor(id(), ProcessStateInfo::Read);
  }
}

bool StreamBufferDescriptor::canRead() {
  upan::mutex_guard g(_ioSync);
  return !_queue.empty();
}

int StreamBufferDescriptor::write(const char* buffer, int len) {
  if (getPid() == NO_PROCESS_ID && id() == IODescriptorTable::STDOUT) {
    KC::MConsole().nMessage(buffer, len, upanui::CharStyle::WHITE_ON_BLACK());
    return len;
  } else {
    while(true) {
      {
        upan::mutex_guard g(_ioSync);
        if (!_queue.full()) {
          return _queue.write(buffer, len);
        }
      }
      if (getMode() & O_WR_NONBLOCK) {
        return 0;
      }
      ProcessManager::Instance().WaitOnIODescriptor(id(), ProcessStateInfo::Write);
    }
  }
}

bool StreamBufferDescriptor::canWrite() {
  upan::mutex_guard g(_ioSync);
  return !_queue.full();
}