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

#include <mosstd.h>
#include <set.h>
#include <map.h>
#include <option.h>
#include <uniq_ptr.h>
#include <atomicop.h>
#include <TaskStructures.h>
#include <FileOperations.h>
#include <ProcessConstants.h>
#include <IODescriptorTable.h>
#include <ProcessGroup.h>
#include <Process.h>

class SchedulableProcess : public Process
{
public:
  typedef upan::set<int> ProcessIDs;

public:
  SchedulableProcess(const upan::string& name, int parentID, bool isFGProcess);
  virtual ~SchedulableProcess() = 0;

  bool isChildThread() const override {
    return _processID != _mainThreadID;
  }

  virtual void onLoad() = 0;

  //thread synchronization mutex
  upan::option<upan::mutex&> heapMutex() override {
    return upan::option<upan::mutex&>::empty();
  }
  virtual upan::option<upan::mutex&> pageAllocMutex() {
    return upan::option<upan::mutex&>::empty();
  }
  upan::option<upan::mutex&> envMutex() override {
    return upan::option<upan::mutex&>::empty();
  }

  virtual SchedulableProcess& forSchedule() {
    throw upan::exception(XLOC, "forSchedule unsupported");
  }

  bool isFGProcessGroup() const override {
    return _processGroup->IsFGProcessGroup();
  }

  uint32_t pdbr() const override {
    return _taskState.CR3_PDBR;
  }

  void Load();
  void Store();
  void Destroy();
  void Release();

  FILE_USER_TYPE fileUserType(const FileSystem::Node&) const override;
  bool hasFilePermission(const FileSystem::Node&, byte mode) const override;

  uint32_t getProcessBase() const override { return _processBase; }
  upan::string name() const { return _name; }
  int processID() const { return _processID; }
  int mainThreadID() const { return _mainThreadID; }

  int parentProcessID() const { return _parentProcessID; }
  void setParentProcessID(int parentProcessID) { _parentProcessID = parentProcessID; }

  bool isDmmFlag() const { return _dmmFlag; }
  void setDmmFlag(bool dmmFlag) { _dmmFlag = dmmFlag; }

  PROCESS_STATUS status() const { return _status; }
  PROCESS_STATUS setStatus(PROCESS_STATUS status) override {
    return (PROCESS_STATUS) upan::atomic::op::swap((__volatile__ uint32_t &) (_status), static_cast<int>(status));
  }

  int driveID() const override { return _driveID; }
  void setDriveID(int driveID) override { _driveID = driveID; }

  int userID() const override { return _userID; }

  ProcessGroup* processGroup() override { return _processGroup; }
  void setProcessGroup(ProcessGroup* processGroup) override { _processGroup = processGroup; }

  ProcessStateInfo& stateInfo() override { return _stateInfo; }
  TaskState& taskState() { return _taskState; }
  ProcessLDT& processLDT() { return _processLDT; }
  FileSystem::PresentWorkingDirectory& processPWD() override { return _processPWD; }
  const FileSystem::PresentWorkingDirectory& processPWD() const override { return _processPWD; }

  const ProcessIDs& childProcessIDs() const { return _childProcessIDs; }
  void addChildProcessID(int pid) { _childProcessIDs.insert(pid); }
  void removeChildProcessID(int pid) { _childProcessIDs.erase(pid); }

private:
  static int _nextPid;

  __inline__ void FXSave();
  __inline__ void FXRestore();

protected:
  virtual void DeAllocateResources() = 0;
  virtual void DestroyThreads() {
  }

  class Common {
  public:
    static uint32_t AllocatePDE();
    static void UpdatePDEWithStackPTE(uint32_t pdeAddress, uint32_t stackPTEAddress);
    static uint32_t AllocatePTEForStack();
    static void AllocateStackSpace(uint32_t pteAddress);
    static void DeAllocateStackSpace(uint32_t stackPTEAddress);
  };

protected:
  upan::string _name;
  int _processID;
  int _mainThreadID;
  int _parentProcessID;
  bool _dmmFlag;
  uint32_t _processBase;
  PROCESS_STATUS _status;
  int _driveID;
  int _userID;
  ProcessStateInfo& _stateInfo;
  TaskState _taskState;
  ProcessLDT _processLDT;
  FileSystem::PresentWorkingDirectory _processPWD;
  //this is managed like a shared_ptr
  ProcessGroup* _processGroup;

  ProcessIDs _childProcessIDs;
  //C++11 -> alignas() or __attribute__((aligned(16)) doesn't work on member attributes of class (in this case Process) that is allocated by new
  //alignas(16) uint8_t _sseRegs[512];// __attribute__((aligned(16)));
  uint8_t* _sseRegs;
};