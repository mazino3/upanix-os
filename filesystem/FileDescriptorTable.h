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

#include <Global.h>
#include <map.h>
#include <uniq_ptr.h>
#include <fs.h>
#include <Atomic.h>
#include <FileDescriptor.h>

class FileDescriptorTable {
public:
  typedef upan::map<int, FileDescriptor*> FDTable;

  FileDescriptorTable();
  ~FileDescriptorTable() noexcept;

  int allocate(const upan::string& fileName, byte mode, int driveID, unsigned fileSize, unsigned startSectorID);
  void free(int fd);
  void updateOffset(int fd, int seekType, int offset);
  uint32_t getOffset(int fd);
  void dup2(int oldFD, int newFD);
  void initStdFile(int stdFD);
  FileDescriptor& getRealNonDupped(int fd);

private:
  FDTable::iterator getItr(int fd);
  FileDescriptor& get(int fd);

  int _fdIdCounter;
  Mutex _fdMutex;
  FDTable _fdTable;
};