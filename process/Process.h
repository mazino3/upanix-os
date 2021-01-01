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
#include <Atomic.h>
#include <TaskStructures.h>
#include <FileOperations.h>

class ProcessGroup;
class IRQ;

typedef struct
{
  char szName[56] ;
  unsigned uiNoOfPages ;
  unsigned* uiAllocatedPageNumbers ;
} PACKED ProcessSharedObjectList ;

class ProcessStateInfo
{
public:
  ProcessStateInfo();

  uint32_t SleepTime() const { return _sleepTime; }
  void SleepTime(const uint32_t s) { _sleepTime = s; }

  int WaitChildProcId() const { return _waitChildProcId; }
  void WaitChildProcId(const int id) { _waitChildProcId = id; }

  RESOURCE_KEYS WaitResourceId() const { return _waitResourceId; }
  void WaitResourceId(const RESOURCE_KEYS id) { _waitResourceId = id; }

  bool IsKernelServiceComplete() const { return _kernelServiceComplete; }
  void KernelServiceComplete(const bool v) { _kernelServiceComplete = v; }

  const IRQ* Irq() const { return _irq; }
  void Irq(const IRQ* irq) { _irq = irq; }

  bool IsEventCompleted();
  void EventCompleted();

private:
  unsigned      _sleepTime ;
  const IRQ*    _irq;
  int           _waitChildProcId ;
  RESOURCE_KEYS _waitResourceId;
  uint32_t      _eventCompleted;
  bool          _kernelServiceComplete ;
};

class Process
{
public:
  Process(const upan::string& name, int parentID, bool isFGProcess);
  virtual ~Process() = 0;

  virtual bool isKernelProcess() const = 0;
  virtual void onLoad() = 0;
  virtual uint32_t startPDEForDLL() const {
    throw upan::exception(XLOC, "startPDEForDLL unsupported");
  }
  virtual void MapDLLPagesToProcess(int dllEntryIndex, uint32_t noOfPagesForDLL, uint32_t allocatedPageCount) {
    throw upan::exception(XLOC, "MapDLLPagesToProcess unsupported");
  }
  virtual uint32_t getAUTAddress() const {
    throw upan::exception(XLOC, "getAUTAddress unsupported");
  }
  virtual void setAUTAddress(uint32_t addr) {
    throw upan::exception(XLOC, "setAUTAddress unsupported");
  }

  uint32_t getProcessBase() {
    return _processBase;
  }

  void Load();
  void Store();
  void Destroy();
  void Release();

  FILE_USER_TYPE FileUserType(const FileSystem::Node&) const;
  bool HasFilePermission(const FileSystem::Node&, byte mode) const;

private:
  static int _nextPid;

protected:
  virtual void DeAllocateResources() = 0;

  uint32_t _processBase;

public:
  upan::string _name;
  bool _dmmFlag;
  int _processID;
  int _mainThreadID;
  ProcessStateInfo& _stateInfo;

  TaskState taskState;
  ProcessLDT processLDT;
  PROCESS_STATUS status;

  int iParentProcessID;

  int iDriveID ;
  FileSystem::PresentWorkingDirectory processPWD ;

  //this is managed like a shared_ptr
  ProcessGroup* _processGroup;

  int iUserID ;
};

//A KernelProcess is similar to a Thread in that they all share same address space (page tables), heap but different stack
//But it is a process in that if the parent process dies before child, then child kernel process will continue to execute under the root kernel process
class KernelProcess : public Process {
public:
  KernelProcess(const upan::string& name, uint32_t taskAddress, int parentID, bool isFGProcess, uint32_t param1, uint32_t param2);

  bool isKernelProcess() const override {
    return true;
  }

  void onLoad() override {}

private:
  void DeAllocateResources() override;
  uint32_t AllocateAddressSpace();

private:
  int kernelStackBlockId;
};

class UserProcess : public Process {
public:
  UserProcess(const upan::string &name, int parentID, int userID, bool isFGProcess, int noOfParams, char** args);

  bool isKernelProcess() const override {
    return false;
  }

  void onLoad() override;

  uint32_t startPDEForDLL() const override {
    return _startPDEForDLL;
  }
  void MapDLLPagesToProcess(int dllEntryIndex, uint32_t noOfPagesForDLL, uint32_t allocatedPageCount) override;
  uint32_t getAUTAddress() const override {
    return _uiAUTAddress;
  }
  void setAUTAddress(uint32_t addr) {
    _uiAUTAddress = addr;
  }

private:
  void Load(int noOfParams, char** szArgumentList);
  uint32_t AllocateAddressSpace();
  void CopyElfImage(unsigned uiPDEAddr, byte* bProcessImage, unsigned uiMemImageSize);
  uint32_t PushProgramInitStackData(int iNumberOfParameters, char** szArgumentList);
  void AllocatePTE(const unsigned uiPDEAddress);
  void InitializeProcessSpaceForOS(const unsigned uiPDEAddress);
  void InitializeProcessSpaceForProcess(const unsigned uiPDEAddress);

  void DeAllocateResources() override;
  void DeAllocateDLLPTEPages();
  void DeAllocateAddressSpace();
  void DeAllocateProcessSpace();
  void DeAllocatePTE();

  uint32_t GetDLLPageAddressForKernel();
  void AllocatePagesForDLL(uint32_t noOfPagesForDLL, ProcessSharedObjectList& pso);

private:
  uint32_t _uiAUTAddress;
  uint32_t _noOfPagesForPTE;
  uint32_t _noOfPagesForProcess;
  uint32_t _uiNoOfPagesForDLLPTE;
  uint32_t _startPDEForDLL;
  uint32_t _stackPTEAddress;
};

class UserThread : public Process {
public:
  UserThread(int parentID, uint32_t entryAddress, void* arg);

  bool isKernelProcess() const override {
    return false;
  }

  void onLoad() override;

  uint32_t startPDEForDLL() const override {
    return _parent.startPDEForDLL();
  }

  void MapDLLPagesToProcess(int dllEntryIndex, uint32_t noOfPagesForDLL, uint32_t allocatedPageCount) override {
    return _parent.MapDLLPagesToProcess(dllEntryIndex, noOfPagesForDLL, allocatedPageCount);
  }

  uint32_t getAUTAddress() const override {
    return _parent.getAUTAddress();
  }

  void setAUTAddress(uint32_t addr) {
    _parent.setAUTAddress(addr);
  }

private:
  uint32_t PushProgramInitStackData(void* arg);
  void DeAllocateResources() override;

private:
  UserProcess& _parent;
  uint32_t _stackPTEAddress;
};
