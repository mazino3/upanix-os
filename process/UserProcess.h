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

#include <list.h>
#include <Process.h>

class UserThread;

class UserProcess : public SchedulableProcess {
public:
  typedef upan::map<upan::string, ProcessDLLInfo> DLLInfoMap;

public:
  UserProcess(const upan::string &name, int parentID, int userID, bool isFGProcess, int noOfParams, char** args);

  bool isKernelProcess() const override {
    return false;
  }

  void onLoad() override;

  uint32_t startPDEForDLL() const override {
    return _startPDEForDLL;
  }

  void LoadELFDLL(const upan::string& szDLLName, const upan::string& szJustDLLName) override;
  void MapDLLPagesToProcess(uint32_t noOfPagesForDLL, const upan::string& dllName) override;
  upan::option<const ProcessDLLInfo&> getDLLInfo(const upan::string& dllName) const override;
  upan::option<const ProcessDLLInfo&> getDLLInfo(int id) const override;

  uint32_t getAUTAddress() const override {
    return _uiAUTAddress;
  }
  void setAUTAddress(uint32_t addr) {
    _uiAUTAddress = addr;
  }

  FileDescriptorTable& fdTable() override {
    return _fdTable;
  }

  upan::option<Mutex&> heapMutex() override {
    return upan::option<Mutex&>(_heapMutex);
  }
  upan::option<Mutex&> pageAllocMutex() override {
    return upan::option<Mutex&>(_pageFaultMutex);
  }
  upan::option<Mutex&> envMutex() override {
    return upan::option<Mutex&>(_envMutex);
  }

  SchedulableProcess& forSchedule() override;
  void addToThreadScheduler(UserThread& thread);

private:
  void Load(int noOfParams, char** szArgumentList);
  uint32_t AllocateAddressSpace();
  void CopyElfImage(unsigned uiPDEAddr, byte* bProcessImage, unsigned uiMemImageSize);
  uint32_t PushProgramInitStackData(int iNumberOfParameters, char** szArgumentList);
  void AllocatePTE(const unsigned uiPDEAddress);
  void InitializeProcessSpaceForOS(const unsigned uiPDEAddress);
  void InitializeProcessSpaceForProcess(const unsigned uiPDEAddress);

  void DestroyThreads() override;
  void DeAllocateResources() override;
  void DeAllocateDLLPages();
  void DeAllocateAddressSpace();
  void DeAllocateProcessSpace();
  void DeAllocatePTE();

private:
  uint32_t _uiAUTAddress;
  uint32_t _noOfPagesForPTE;
  uint32_t _noOfPagesForProcess;
  uint32_t _noOfPagesForDLLPTE;
  uint32_t _totalNoOfPagesForDLL;
  uint32_t _startPDEForDLL;
  uint32_t _stackPTEAddress;
  upan::vector<upan::string> _loadedDLLs;
  DLLInfoMap _dllInfoMap;
  Mutex _heapMutex;
  Mutex _pageFaultMutex;
  Mutex _envMutex;
  FileDescriptorTable _fdTable;

  typedef upan::list<UserThread*> ThreadSchedulerList;
  ThreadSchedulerList _threadSchedulerList;
  ThreadSchedulerList::iterator _nextThreadIt;
};