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

#include <Process.h>
#include <RootGUIConsole.h>
#include <IODescriptorTable.h>
#include <drive.h>
#include <UserManager.h>
#include <MemManager.h>

class KernelRootProcess : public Process {
private:
  KernelRootProcess() : _iodTable(NO_PROCESS_ID, NO_PROCESS_ID) {
  }

public:
  static KernelRootProcess& Instance() {
    static KernelRootProcess instance;
    return instance;
  }

  bool isKernelProcess() const override {
    return true;
  }

  bool isFGProcessGroup() const override {
    return true;
  }

  int processID() const override {
    return NO_PROCESS_ID;
  }

  int parentProcessID() const override {
    return NO_PROCESS_ID;
  }

  int driveID() const override {
    return ROOT_DRIVE;
  }

  uint32_t getProcessBase() const override {
    return GLOBAL_DATA_SEGMENT_BASE;
  }

  int userID() const override {
    return ROOT_USER_ID;
  }

  bool isChildThread() const override {
    return false;
  }

  upan::option<upan::mutex&> envMutex() override {
    return upan::option<upan::mutex&>(_envMutex);
  }

  IODescriptorTable& iodTable() override {
    return _iodTable;
  }

  FILE_USER_TYPE fileUserType(const FileSystem::Node&) const override {
    return USER_OWNER;
  }

  bool hasFilePermission(const FileSystem::Node&, byte mode) const override {
    return true;
  }

  uint32_t pdbr() const override {
    return MEM_PDBR;
  }

  void setDriveID(int driveID) override {
    throw upan::exception(XLOC, "setDriveID() unsupported");
  }

  FileSystem::PresentWorkingDirectory& processPWD() override {
    throw upan::exception(XLOC, "processPWD() unsupported");
  }

  const FileSystem::PresentWorkingDirectory& processPWD() const override {
    throw upan::exception(XLOC, "processPWD() const unsupported");
  }

  ProcessStateInfo& stateInfo() {
    throw upan::exception(XLOC, "stateInfo() unsupported");
  }

  PROCESS_STATUS setStatus(PROCESS_STATUS status) {
    throw upan::exception(XLOC, "setStatus() unsupported");
  }

  ProcessGroup* processGroup() override {
    throw upan::exception(XLOC, "processGroup() unsupported");
  }

  void setProcessGroup(ProcessGroup* processGroup) {
    throw upan::exception(XLOC, "setProcessGroup() unsupported");
  }

  upan::option<RootFrame&> getGuiFrame() override {
    return upan::option<RootFrame&>(RootGUIConsole::Instance().frame());
  }

  void initGuiFrame() override;

private:
  upan::mutex _envMutex;
  upan::mutex _fdMutex;
  IODescriptorTable _iodTable;
};