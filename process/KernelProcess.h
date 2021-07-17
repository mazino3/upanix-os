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

#include <AutonomousProcess.h>
#include <KernelThread.h>
#include <vector.h>

//A KernelProcess is similar to a Thread in that they all share same address space (page tables), heap but different stack
//But it is a process in that if the parent process dies before child, then child kernel process will continue to execute under the root kernel process
class KernelProcess : public AutonomousProcess {
public:
  KernelProcess(const upan::string& name, uint32_t taskAddress, int parentID, bool isFGProcess, const upan::vector<uint32_t>& params);

  bool isKernelProcess() const override {
    return true;
  }

  void onLoad() override {}

  KernelThread& CreateThread(uint32_t threadCaller, uint32_t entryAddress, void* arg) override;

  upan::option<upan::mutex&> envMutex() override {
    return upan::option<upan::mutex&>(_envMutex);
  }

  FileDescriptorTable& fdTable() override {
    return _fdTable;
  }

  uint32_t getGUIFramebufferAddress() override;

private:
  void DeAllocateResources() override;
  uint32_t AllocateAddressSpace();
  void DeAllocateGUIFramebuffer();

private:
  int kernelStackBlockId;
  FileDescriptorTable _fdTable;

  //common mutex for all kernel processes
  static upan::mutex _envMutex;
};