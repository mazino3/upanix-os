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

class RedirectDescriptor : public IODescriptor {
public:
  RedirectDescriptor(int pid, int id, IODescriptor& parentDesc);

  upan::option<IODescriptor&> getParentDescriptor() override;

  IODescriptor& getRealDescriptor() override {
    return getParentDescriptor().value().getRealDescriptor();
  }

  int read(char* buffer, int len) override {
    return getParentDescriptor().value().read(buffer, len);
  }

  bool canRead() override {
    return getParentDescriptor().value().canRead();
  }

  int write(const char* buffer, int len) override {
    return getParentDescriptor().value().write(buffer, len);
  }

  bool canWrite() override {
    return getParentDescriptor().value().canWrite();
  }

  void seek(int seekType, int offset) override {
    getParentDescriptor().value().seek(seekType, offset);
  }

  uint32_t getOffset() const override {
    return const_cast<RedirectDescriptor&>(*this).getParentDescriptor().value().getOffset();
  }

  uint32_t getSize() const override {
    return const_cast<RedirectDescriptor&>(*this).getParentDescriptor().value().getSize();
  }
private:
  const int _parentPid;
  const int _parentDescId;
};
