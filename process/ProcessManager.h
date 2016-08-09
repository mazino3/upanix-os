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
#ifndef _PROCESS_MANAGER_H_
#define _PROCESS_MANAGER_H_

#include <Global.h>
#include <MemConstants.h>
#include <FileSystem.h>
#include <ElfSectionHeader.h>
#include <PIC.h>
#include <Atomic.h>
#include <list.h>

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

typedef enum
{
	NEW,
	RUN,
	WAIT_SLEEP,
	WAIT_INT,
	WAIT_CHILD,
	WAIT_RESOURCE,
	WAIT_KERNEL_SERVICE,
  WAIT_EVENT,
	TERMINATED,
} PROCESS_STATUS ;

class ProcessGroup;

typedef struct
{
	unsigned short	backlink ;
	unsigned short	FILLER1 ;
	
	unsigned		ESP0 ;
	unsigned short	SS0 ;
	unsigned short	FILLER2 ;

	unsigned		ESP1 ;
	unsigned short	SS1 ;
	unsigned short	FILLER3 ;

	unsigned		ESP2 ;
	unsigned short	SS2 ;
	unsigned short	FILLER4 ;

	unsigned		CR3_PDBR ;
	unsigned		EIP ;
	unsigned		EFLAGS ;
	
	unsigned		EAX ;
	unsigned		ECX ;
	unsigned		EDX ;
	unsigned		EBX ;
	unsigned		ESP ;
	unsigned		EBP ;
	unsigned		ESI ;
	unsigned		EDI ;
	
	unsigned short	ES ;
	unsigned short	FILLER5 ;

	unsigned short	CS ;
	unsigned short	FILLER6 ;

	unsigned short	SS ;
	unsigned short	FILLER7 ;

	unsigned short	DS ;
	unsigned short	FILLER8 ;

	unsigned short	FS ;
	unsigned short	FILLER9 ;

	unsigned short	GS ;
	unsigned short	FILLER10 ;

	unsigned short	LDT ;
	unsigned short	FILLER11 ;

	unsigned short	DEBUG_T_BIT ;
	unsigned short	IO_MAP_BASE ;
} PACKED TaskState ;

typedef struct
{
	unsigned short	usLimit15_0;
	unsigned short	usBase15_0 ;
	byte			bBase23_16 ;
	byte			bType ;
	byte			bLimit19_16_Flags ;
	byte			bBase31_24 ;
} PACKED Descriptor ;

typedef struct
{	
	unsigned short int lowerOffset ;	
	unsigned short int selector ;
	byte parameterCount ;
	byte options ; /*	constant:5;	 5 bits
					 dpl:2;		 2 bits
					 present:1;	 1 bit		*/
	unsigned short int upperOffset ;
} PACKED GateDescriptor ;

typedef struct
{
	Descriptor NullDesc ;
	Descriptor LinearDesc ;
	Descriptor CodeDesc ;
	Descriptor DataDesc ;
	Descriptor StackDesc ;
	Descriptor CallGateStackDesc ;
} PACKED ProcessLDT ;

typedef struct
{
	unsigned uiSleepTime ;
	
	const IRQ* pIRQ ;

	int iWaitChildProcId ;

	RESOURCE_KEYS uiWaitResourceId;

  uint32_t _eventCompleted;

	bool bKernelServiceComplete ;
		
} ProcessStateInfo ;

typedef struct
{
	TaskState taskState ;
	ProcessLDT processLDT ;
	byte bFree ;
	PROCESS_STATUS status ;
	ProcessStateInfo* pProcessStateInfo ;

	int iParentProcessID ;
	int iNextProcessID ;
	int iPrevProcessID ;

	unsigned uiNoOfPagesForPTE ;
	unsigned uiNoOfPagesForProcess ;

	byte bIsKernelProcess ;
	int iKernelStackBlockID ;

	unsigned uiAUTAddress ;
	unsigned uiProcessBase ;

	int iDriveID ;
	FileSystem_PresentWorkingDirectory processPWD ;

	unsigned uiNoOfPagesForDLLPTE ;
	unsigned uiStartPDEForDLL ;
	
  ProcessGroup* _processGroup;

	int iUserID ;

	char* pname ;
	
	unsigned uiPageFaultAddress ;
	unsigned uiDMMFlag ;
} PACKED ProcessAddressSpace ;

extern unsigned ProcessManager_uiKernelAUTAddress ;

typedef struct
{
	int pid ;
	char* pname ;
	PROCESS_STATUS status ;
	int iParentProcessID ;
	int iProcessGroupID ;
	int iUserID ;
} PS ;

void ProcessManager_ProcessInit() ;
void ProcessManager_ProcessUnInit() ;
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
    ProcessAddressSpace& GetAddressSpace(int pid);
    PS* GetProcList(unsigned& uiListSize);
    void FreeProcListMem(PS* pProcList, unsigned uiListSize);
    ProcessAddressSpace& GetCurrentPAS() { return _processAddressSpace[_iCurrentProcessID]; }
    int FindFreePAS();
    void StartScheduler();
    void DeleteFromSchedulerList(int iDeleteProcessID);
    void AddToSchedulerList(int iNewProcessID);
    void InsertIntoProcessList(int iProcessID);
    void DeleteFromProcessList(int iProcessID);
    bool WakeupProcessOnInterrupt(__volatile__ int iProcessID);
    bool IsResourceBusy(__volatile__ RESOURCE_KEYS uiType);
    void SetResourceBusy(RESOURCE_KEYS uiType, bool bVal);
    void Sleep(unsigned uiSleepTime);
    void WaitOnInterrupt(const IRQ&);
    int GetCurProcId();
    bool CopyDiskDrive(int iProcessID, int& iOldDriveId, FileSystem_PresentWorkingDirectory& mOldPWD);
    void Kill(int iProcessID);
    void WakeUpFromKSWait(int iProcessID);
    bool IsChildAlive(int iChildProcessID);
    byte CreateKernelImage(const unsigned uiTaskAddress, int iParentProcessID, byte bIsFGProcess, unsigned uiParam1, unsigned uiParam2, int* iProcessID, const char* szKernelProcName);
    byte Create(const char* szProcessName, int iParentProcessID, byte bIsFGProcess, int* iProcessID, int iUserID, int iNumberOfParameters, char** szArgumentList);
    void SetDMMFlag(int iProcessID, bool flag);
    bool IsDMMOn(int iProcessID);
    void WaitOnChild(int iChildProcessID);
    void WaitOnResource(RESOURCE_KEYS uiResourceType);
    void WaitOnKernelService();
    bool IsKernelProcess(int iProcessID);
    void BuildCallGate(unsigned short usGateSelector, unsigned uiOffset, unsigned short usSelector, byte bParameterCount);
    bool ConditionalWait(const unsigned* registry, unsigned bitPos, bool waitfor);
    void WaitForEvent();
    void EventCompleted(int pid);

    static int GetCurrentProcessID() { return _iCurrentProcessID; }
    static void EnableTaskSwitch() ;
    static void DisableTaskSwitch() ;
  private:
    void Destroy(int iDeleteProcessID);
    void DoContextSwitch(int iProcessID);
    PS* GetProcListASync();
    void Load(int iProcessID);
    void Store(int iProcessID);
    void DeAllocateProcessInitDockMem(ProcessAddressSpace& pas);
    void DeAllocateResources(int iProcessID);
    void Release(int iProcessID);
    bool DoPollWait();
    void BuildDescriptor(Descriptor* descriptor, unsigned uiLimit, unsigned uiBase, byte bType, byte bFlag);
    void BuildLDT(ProcessLDT* processLDT);
    void BuildKernelTaskLDT(int iProcessID);
    void BuildIntTaskState(const unsigned uiTaskAddress, const unsigned uiTSSAddress, const int stack);
    void BuildTaskState(ProcessAddressSpace* pProcessAddressSpace, unsigned uiPDEAddress, unsigned uiEntryAdddress, unsigned uiProcessEntryStackSize);
    void BuildKernelTaskState(const unsigned uiTaskAddress, const int iProcessID, const unsigned uiStackTop, unsigned uiParam1, unsigned uiParam2);
    void BuildIntGate(unsigned short usGateSelector, unsigned uiOffset, unsigned short usSelector, byte bParameterCount);
    bool IsEventCompleted(ProcessAddressSpace& pas);

    static int _iCurrentProcessID;
  
    upan::list<int> _processList;
    bool _resourceList[MAX_RESOURCE];

    unsigned _uiProcessCount;
    ProcessAddressSpace* _processAddressSpace;
};

class ProcessSwitchLock
{
  public:
    ProcessSwitchLock() : _locked(true)
    {
	    ProcessManager::DisableTaskSwitch();
    }

    ~ProcessSwitchLock()
    {
      UnLock();
    }

    void UnLock()
    {
      if(_locked)
      {
        ProcessManager::EnableTaskSwitch();
        _locked = false;
      }
      else
        printf("\n unlocking process switch which is already unlocked!!");
    }
  private:
    bool _locked;
};

#endif

