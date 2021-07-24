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
#include <mutex.h>
#include <IODescriptor.h>
#include <function.h>

class IODescriptorTable {
public:
  typedef enum {
    STDIN = 0,
    STDOUT = 1,
    STDERR = 2
  } STD_DESCRIPTORS;

  typedef upan::map<int, IODescriptor*> IODMap;

  IODescriptorTable();
  ~IODescriptorTable() noexcept;

  IODescriptor& allocate(const upan::function<IODescriptor*, int>& descriptorBuilder);
  void free(int fd);
  void dup2(int oldFD, int newFD);
  IODescriptor& getRealNonDupped(int fd);

private:
  IODMap::iterator getItr(int fd);
  IODescriptor& get(int fd);

  int _fdIdCounter;
  upan::mutex _fdMutex;
  IODMap _iodMap;
};