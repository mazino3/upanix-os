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

#include <stdlib.h>
#include <option.h>

class IODescriptor {
protected:
  IODescriptor(int id, byte mode) : _id(id), _mode(mode), _refCount(1) {
  }

public:
  virtual ~IODescriptor() {}

  int id() {
    return _id;
  }

  virtual IODescriptor& getRealDescriptor() {
    return *this;
  }

  virtual upan::option<IODescriptor&> getParentDescriptor() {
    return upan::option<IODescriptor&>::empty();
  }

  byte getMode() const {
    return _mode;
  }

  int getRefCount() const {
    return _refCount;
  }

  void decrementRefCount() {
    --_refCount;
  }

  void incrementRefCount() {
    ++_refCount;
  }

  virtual int read(char* buffer, int len) = 0;
  virtual int write(const char* buffer, int len) = 0;
  virtual void seek(int seekType, int offset) = 0;
  virtual uint32_t getOffset() const = 0;
  virtual uint32_t getSize() const = 0;

private:
  int _id;
  byte _mode;
  int _refCount;
};