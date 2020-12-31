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
#include <mosstd.h>
#include <list.h>
#include <map.h>
#include <option.h>
#include <MemConstants.h>
#include <FileSystem.h>
#include <ElfSectionHeader.h>
#include <PIC.h>
#include <Atomic.h>
#include <Process.h>

#include <PIT.h>

#define ProcessManager_SUCCESS						0
#define ProcessManager_ERR_MAX_PROCESS_EXCEEDED		1
#define ProcessManager_ERR_NO_FREE_PAS				2
#define ProcessManager_ERR_INT_QUEUE_EMPTY			3
#define ProcessManager_ERR_INT_QUEUE_FULL			4
#define ProcessManager_FAILURE						5

#define ProcessManager_EXIT() \
	__asm__ __volatile__("pusha") ; \
	__asm__ __volatile__("pushf") ; \
	__asm__ __volatile__("popl %eax") ; \
	__asm__ __volatile__("mov $0x4000, %ebx") ; \
	__asm__ __volatile__("or %ebx, %eax") ; \
	__asm__ __volatile__("pushl %eax") ; \
	__asm__ __volatile__("popf") ; \
	__asm__ __volatile__("popa") ; \
	__asm__ __volatile__("iret") 
	
#define ProcessManager_RESTORE() \
	__asm__ __volatile__("pusha") ; \
	__asm__ __volatile__("pushf") ; \
	__asm__ __volatile__("popl %eax") ; \
	__asm__ __volatile__("mov $0xBFFF, %ebx") ; \
	__asm__ __volatile__("and %ebx, %eax") ; \
	__asm__ __volatile__("pushl %eax") ; \
	__asm__ __volatile__("popf") ; \
	__asm__ __volatile__("popa") ; \
	__asm__ __volatile__("leave") ; \
	__asm__ __volatile__("ret") ;

void ProcessManager_Exit() ;

class ProcessManager
{
  private:
    ProcessManager();
  public:
    static ProcessManager& Instance()
    {
      static ProcessManager instance;
      return instance;
    }

    upan::option<UserProcess&> GetUserProcess(int pid);
    upan::option<Process&> GetAddressSpace(int pid);
    Process& GetCurrentPAS();

    PS* GetProcList(unsigned& uiListSize);
    void FreeProcListMem(PS* pProcList, unsigned uiListSize);
    void StartScheduler();
    void AddToSchedulerList(Process& process);
    bool WakeupProcessOnInterrupt(Process& process);
    bool IsResourceBusy(__volatile__ RESOURCE_KEYS uiType);
    void SetResourceBusy(RESOURCE_KEYS uiType, bool bVal);
    void Sleep(unsigned uiSleepTime);
    void WaitOnInterrupt(const IRQ&);
    int GetCurProcId();
    bool CopyDiskDrive(int iProcessID, int& iOldDriveId, FileSystem::PresentWorkingDirectory& mOldPWD);
    void Kill(int iProcessID);
    void WakeUpFromKSWait(int iProcessID);
    bool IsChildAlive(int iChildProcessID);
    byte CreateKernelProcess(const upan::string& name, const unsigned uiTaskAddress, int iParentProcessID, byte bIsFGProcess, unsigned uiParam1, unsigned uiParam2, int* iProcessID);
    byte Create(const upan::string& name, int iParentProcessID, byte bIsFGProcess, int* iProcessID, int iUserID, int iNumberOfParameters, char** szArgumentList);
    bool CreateThreadTask(int parentID, uint32_t threadEntryAddress, bool isFGProcess, void* arg, int& threadID);
    void SetDMMFlag(int iProcessID, bool flag);
    bool IsDMMOn(int iProcessID);
    void WaitOnChild(int iChildProcessID);
    void WaitOnResource(RESOURCE_KEYS uiResourceType);
    void WaitOnKernelService();
    bool IsKernelProcess(int iProcessID);
    void BuildCallGate(unsigned short usGateSelector, unsigned uiOffset, unsigned short usSelector, byte bParameterCount);
    bool ConditionalWait(const volatile unsigned* registry, unsigned bitPos, bool waitfor);
    void WaitForEvent();
    void EventCompleted(int pid);

    static int GetCurrentProcessID() {
      return _currentProcessID;
    }
    static void EnableTaskSwitch() ;
    static void DisableTaskSwitch() ;
  private:
    void DoContextSwitch(Process& process);
    void Destroy(Process& pas);
    bool DoPollWait();
    void BuildIntTaskState(const unsigned uiTaskAddress, const unsigned uiTSSAddress, const int stack);
    void BuildIntGate(unsigned short usGateSelector, unsigned uiOffset, unsigned short usSelector, byte bParameterCount);
    bool IsEventCompleted(int pid);
    ProcessStateInfo& GetProcessStateInfo(int pid);

    bool _resourceList[MAX_RESOURCE];

    ProcessStateInfo _kernelModeStateInfo;
    typedef upan::map<int, Process*> ProcessMap;
    ProcessMap _processMap;
    ProcessMap::iterator _currentProcessIt;
    //This is required even before initializing the ProcessManager for fetching
    static int _currentProcessID;
};

class ProcessSwitchLock {
public:
  ProcessSwitchLock() : _isOwner(false) {
    _isOwner = PIT_DisableTaskSwitch();
  }

  ~ProcessSwitchLock() {
    if (_isOwner) {
      PIT_EnableTaskSwitch();
    }
  }

private:
  bool _isOwner;
};
