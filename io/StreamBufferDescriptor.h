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

#include <IODescriptor.h>
#include <uniq_ptr.h>
#include <queue.h>

class StreamBufferDescriptor : public IODescriptor {
public:
  StreamBufferDescriptor(int pid, int id, uint32_t bufSize);

  int read(char* buffer, int len) override;
  int write(const char* buffer, int len) override;

  void seek(int seekType, int offset) override {
    //no-op;
  }

  uint32_t getOffset() const override {
    return 0;
  }

  uint32_t getSize() const override {
    return _queue.size();
  }

private:
  upan::queue<char> _queue;
};
