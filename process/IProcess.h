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

#include <option.h>
#include <FileSystem.h>
#include <ProcessStateInfo.h>
#include <ProcessDLLInfo.h>

class Process {
public:
  virtual bool isKernelProcess() const = 0;
  virtual bool isFGProcessGroup() const = 0;
  virtual int driveID() const = 0;
  virtual uint32_t getProcessBase() const = 0;
  virtual int userID() const = 0;

  virtual FILE_USER_TYPE fileUserType(const FileSystem::Node&) const = 0;
  virtual bool hasFilePermission(const FileSystem::Node&, byte mode) const = 0;
  virtual uint32_t pdbr() const = 0;

  virtual void setDriveID(int driveID) = 0;
  virtual FileSystem::PresentWorkingDirectory& processPWD() = 0;
  virtual const FileSystem::PresentWorkingDirectory& processPWD() const = 0;
  virtual ProcessStateInfo& stateInfo() = 0;
  virtual PROCESS_STATUS setStatus(PROCESS_STATUS status) = 0;
  virtual ProcessGroup* processGroup() = 0;
  virtual void setProcessGroup(ProcessGroup* processGroup) = 0;

  virtual FileDescriptorTable& fdTable() = 0;
  virtual upan::option<Mutex&> envMutex() = 0;
  virtual upan::option<Mutex&> heapMutex() {
    return upan::option<Mutex&>::empty();
  }

  virtual uint32_t startPDEForDLL() const {
    throw upan::exception(XLOC, "startPDEForDLL unsupported");
  }

  virtual void LoadELFDLL(const upan::string& szDLLName, const upan::string& szJustDLLName) {
    throw upan::exception(XLOC, "LoadELFDLL unsupported");
  }

  virtual void MapDLLPagesToProcess(uint32_t noOfPagesForDLL, const upan::string& dllName) {
    throw upan::exception(XLOC, "MapDLLPagesToProcess unsupported");
  }

  virtual upan::option<const ProcessDLLInfo&> getDLLInfo(const upan::string& dllName) const {
    throw upan::exception(XLOC, "getDLLInfo unsupported");
  }

  virtual upan::option<const ProcessDLLInfo&> getDLLInfo(int id) const {
    throw upan::exception(XLOC, "getDLLInfo unsupported");
  }

  virtual uint32_t getAUTAddress() const {
    throw upan::exception(XLOC, "getAUTAddress unsupported");
  }

  virtual void setAUTAddress(uint32_t addr) {
    throw upan::exception(XLOC, "setAUTAddress unsupported");
  }
};