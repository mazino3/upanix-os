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
#include <IODescriptor.h>

class DupDescriptor : public IODescriptor {
public:
  DupDescriptor(int id, IODescriptor& parentDesc) : IODescriptor(id, parentDesc.getMode()), _parentDesc(parentDesc) {
    if (parentDesc.id() == id) {
      throw upan::exception(XLOC, "invalid DupDescriptor - id same as parent id: %d", id);
    }
  }

  IODescriptor& getRealDescriptor() override {
    return _parentDesc.getRealDescriptor();
  }

  upan::option<IODescriptor&> getParentDescriptor() override {
    return upan::option<IODescriptor&>(_parentDesc);
  }

  int read(char* buffer, int len) override {
    return _parentDesc.read(buffer, len);
  }

  int write(const char* buffer, int len) override {
    return _parentDesc.write(buffer, len);
  }

  void seek(int seekType, int offset) override {
    _parentDesc.seek(seekType, offset);
  }

  uint32_t getOffset() const override {
    return _parentDesc.getOffset();
  }

  uint32_t getSize() const override {
    return _parentDesc.getSize();
  }

private:
  IODescriptor& _parentDesc;
};
