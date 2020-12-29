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
#include <ProcessManager.h>
#include <Display.h>
#include <MemUtil.h>
#include <MemManager.h>
#include <FileSystem.h>
#include <Directory.h>
#include <IrqManager.h>
#include <PIT.h>
#include <AsmUtil.h>
#include <ProcessLoader.h>
#include <ProcessAllocator.h>
#include <DynamicLinkLoader.h>
#include <DMM.h>
#include <DLLLoader.h>
#include <ProcessEnv.h>
#include <KernelService.h>
#include <ProcFileManager.h>
#include <UserManager.h>
#include <ProcessGroup.h>
#include <MountManager.h>
#include <StringUtil.h>
#include <UpanixMain.h>
#include <ProcessConstants.h>
#include <KernelUtil.h>
#include <exception.h>
#include <Atomic.h>
#include <uniq_ptr.h>
#include <syscalldefs.h>

int ProcessManager::_currentProcessID = NO_PROCESS_ID;
static const char* DEF_KERNEL_PROC_NAME = "Kernel Proc" ;

static void ProcessManager_Yield()
{
	PIT_SetContextSwitch(true) ;
	ProcessManager_EXIT() ;
	ProcessManager_RESTORE() ;	
}

ProcessManager::ProcessManager() : _currentProcessIt(_processMap.end()) {
  for(int i = 0; i < MAX_RESOURCE; i++)
    _resourceList[i] = false ;

  PIT_SetContextSwitch(false) ;

	TaskState* sysTSS = (TaskState*)(MEM_SYS_TSS_START - GLOBAL_DATA_SEGMENT_BASE) ;
  memset(sysTSS, 0, sizeof(TaskState));
	sysTSS->CR3_PDBR = MEM_PDBR ;
	sysTSS->DEBUG_T_BIT = 0 ;
	sysTSS->IO_MAP_BASE = 103 ;

  ProcessLoader::Instance();

	DLLLoader_Initialize() ;	

	KC::MDisplay().LoadMessage("Process Manager Initialization", Success);
}

upan::option<Process&> ProcessManager::GetAddressSpace(int pid) {
  ProcessSwitchLock switchLock;
  auto it = _processMap.find(pid);
  if (it == _processMap.end()) {
    return upan::option<Process&>::empty();
  }
  return upan::option<Process&>(*it->second);
}

Process& ProcessManager::GetCurrentPAS() {
  ProcessSwitchLock switchLock;
  //This function is a utility that assumes that a ProcessAddressSpace entry always exists for current (active) process
  //Caller should take care of cases when ProcessID = NO_PROCESS_ID - which is usually the case before kernel scheduler is initialized
  return GetAddressSpace(_currentProcessIt->first).value();
}

void ProcessManager::BuildCallGate(unsigned short usGateSelector, unsigned uiOffset, unsigned short usSelector, byte bParameterCount)
{
	GateDescriptor* gateEntry = (GateDescriptor*)(MEM_GDT_START + usGateSelector) ;
	
	__asm__ __volatile__("push %ds") ;
	MemUtil_SetDS(SYS_LINEAR_SELECTOR_DEFINED) ;

	gateEntry->lowerOffset = uiOffset & 0x0000FFFF ;
	gateEntry->selector = usSelector ;
	gateEntry->parameterCount = bParameterCount & 0x1F ;
	gateEntry->options = 0xEC ; // 11101100 --> 1 = Present. 11 = DPL. 01100 = CallGateType 
	gateEntry->upperOffset = (uiOffset & 0xFFFF0000) >> 16 ;

	__asm__ __volatile__("pop %ds") ;
}

void ProcessManager::BuildIntGate(unsigned short usGateSelector, unsigned uiOffset, unsigned short usSelector, byte bParameterCount)
{
	GateDescriptor* gateEntry = (GateDescriptor*)(MEM_GDT_START + usGateSelector) ;
	
	__asm__ __volatile__("push %ds") ;
	MemUtil_SetDS(SYS_LINEAR_SELECTOR_DEFINED) ;

	gateEntry->lowerOffset = uiOffset & 0x0000FFFF ;
	gateEntry->selector = usSelector ;
	gateEntry->parameterCount = bParameterCount & 0x1F ;
	gateEntry->options = 0xEE ; // 11101110 --> 1 = Present. 11 = DPL. 01110 = IntGateType 
	
	gateEntry->upperOffset = (uiOffset & 0xFFFF0000) >> 16 ;

	__asm__ __volatile__("pop %ds") ;
}

Process* ProcessManager::GetNewPAS() {
  static Mutex m;
  MutexGuard g(m);

  if(_processMap.size() + 1 > MAX_NO_PROCESS)
    throw upan::exception(XLOC, "Max process limit reached!");

  return new Process();
}

void ProcessManager::BuildIntTaskState(const unsigned uiTaskAddress, const unsigned uiTSSAddress, const int stack)
{
	TaskState* taskState = (TaskState*)(uiTSSAddress - GLOBAL_DATA_SEGMENT_BASE) ;
	memset(taskState, 0, sizeof(TaskState)) ;

	taskState->EIP = uiTaskAddress ;
	taskState->ESP = stack ;

	taskState->ES = 0x8 ;

	taskState->CS = 0x10 ;
	taskState->DS = 0x18 ;
	taskState->SS = 0x18 ;
	taskState->FS = 0x18 ;
	taskState->GS = 0x18 ;
	taskState->LDT = 0x0 ;

	taskState->CR3_PDBR = MEM_PDBR ;
	taskState->EFLAGS = 0x202 ;
	taskState->DEBUG_T_BIT = 0x00 ;
	taskState->IO_MAP_BASE = 103 ; // > TSS Limit => No I/O Permission Bit Map present
}

void ProcessManager::BuildKernelTaskState(const unsigned uiTaskAddress, TaskState& taskState, const unsigned uiStackTop, unsigned uiParam1, unsigned uiParam2) {
	memset(&taskState, 0, sizeof(TaskState));

	taskState.EIP = uiTaskAddress ;

	((unsigned*)(uiStackTop - 8))[0] = uiParam1 ;
	((unsigned*)(uiStackTop - 8))[1] = uiParam2 ;
	taskState.ESP = uiStackTop - 12 ;

	taskState.ES = 0x8 | 0x4 ;

	taskState.CS = 0x10 | 0x4 ;

	taskState.DS = 0x18 | 0x4 ;
	taskState.FS = 0x18 | 0x4 ;
	taskState.GS = 0x18 | 0x4 ;

	taskState.SS = 0x20 | 0x4 ;

	taskState.LDT = 0x50 ;

	taskState.CR3_PDBR = MEM_PDBR ;
	taskState.EFLAGS = 0x202 ;
	taskState.DEBUG_T_BIT = 0x00 ;
	taskState.IO_MAP_BASE = 103 ; // > TSS Limit => No I/O Permission Bit Map present
}

void ProcessManager::AddToSchedulerList(Process& process) {
  ProcessSwitchLock switchLock;
  _processMap.insert(ProcessMap::value_type(process._processID, &process));
}

void ProcessManager::Destroy(Process& pas) {
	Atomic::Swap((__volatile__ uint32_t&)(pas.status), static_cast<int>(TERMINATED)) ;
	//MemManager::Instance().DisplayNoOfFreePages();
	//MemManager::Instance().DisplayNoOfAllocPages(0,0);

	// Deallocate Resources
	DeAllocateResources(pas) ;

  // Release From Process Group
  pas._processGroup->RemoveFromFGProcessList(pas._processID);
  pas._processGroup->RemoveProcess();

  if(pas._processGroup->Size() == 0)
    delete pas._processGroup;

	// Child processes of this process (if any) will be redirected to "kernel" process
	for(auto p : _processMap) {
		if (p.first == pas._processID)  {
      continue;
		}

    Process* pc = p.second;
		if(pc->iParentProcessID == pas._processID) {
			if(pc->status == TERMINATED) {
				Release(*pc) ;
			}	else {
				pc->iParentProcessID = UpanixMain_KernelProcessID() ;
			}
		}
	}

	if(pas.iParentProcessID == NO_PROCESS_ID)
		Release(pas) ;

	//MemManager::Instance().DisplayNoOfFreePages();
	//MemManager::Instance().DisplayNoOfAllocPages(0,0);
}

void ProcessManager::Load(Process& process) {
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&process.processLDT, SYS_LINEAR_SELECTOR_DEFINED, MEM_LDT_START, sizeof(ProcessLDT)) ;
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&process.taskState, SYS_LINEAR_SELECTOR_DEFINED, MEM_USER_TSS_START, sizeof(TaskState)) ;
}

void ProcessManager::Store(Process& process) {
	MemUtil_CopyMemory(SYS_LINEAR_SELECTOR_DEFINED, MEM_LDT_START, MemUtil_GetDS(), (unsigned)&process.processLDT, sizeof(ProcessLDT)) ;
	MemUtil_CopyMemory(SYS_LINEAR_SELECTOR_DEFINED, MEM_USER_TSS_START,	MemUtil_GetDS(), (unsigned)&process.taskState, sizeof(TaskState)) ;
}

void ProcessManager::DoContextSwitch(Process& process) {
  Process* pPAS = &process;
  const int iProcessID = process._processID;
  ProcessStateInfo* pStat = pPAS->pProcessStateInfo;

	switch(pPAS->status) {
	  case RELEASED:
	    return;
	  case TERMINATED:
	    if (pPAS->iParentProcessID == UpanixMain_KernelProcessID()) {
	      Release(*pPAS);
	    }
	    return;
  	case WAIT_SLEEP:
		{
      if(PIT_GetClockCount() >= pStat->SleepTime())
			{
        pStat->SleepTime(0) ;
				pPAS->status = RUN ;
			}
			else
			{
				if(debug_point) 
				{
          printf("\n Sleep Time: %u", pStat->SleepTime()) ;
					printf("\n PIT Tick Count: %u", PIT_GetClockCount()) ;
					printf("\n") ;
				}
				return ;
			}
		}
		break ;

	  case WAIT_INT:
		{
			if(!WakeupProcessOnInterrupt(*pPAS))
				return ;
		}
		break ;

    case WAIT_EVENT:
    {
      if(!IsEventCompleted(pPAS->_processID))
        return;
      pPAS->status = RUN;
    }
    break;
	
	  case WAIT_CHILD:
		{
      if(pStat->WaitChildProcId() < 0)
			{
        pStat->WaitChildProcId(NO_PROCESS_ID);
				pPAS->status = RUN ;
			}
			else
			{
			  auto childProcess = GetAddressSpace(pStat->WaitChildProcId());
				if(childProcess.isEmpty() || childProcess.value().iParentProcessID != iProcessID)
				{
          pStat->WaitChildProcId(NO_PROCESS_ID);
					pPAS->status = RUN ;
				}
				else if(childProcess.value().status == TERMINATED && childProcess.value().iParentProcessID == iProcessID)
				{
          Release(childProcess.value()) ;
          pStat->WaitChildProcId(NO_PROCESS_ID);
					pPAS->status = RUN ;
				}
				else
					return ;
			}
		}
		break ;

	  case WAIT_RESOURCE:
		{
      if(pStat->WaitResourceId() == RESOURCE_NIL)
			{
				pPAS->status = RUN ;
			}
			else
			{
        if(_resourceList[pStat->WaitResourceId()] == false)
				{ 
          pStat->WaitResourceId(RESOURCE_NIL);
					pPAS->status = RUN ;
				}
				else
					return ;
			}
		}
		break ;

	  case WAIT_KERNEL_SERVICE:
		{
      if(pPAS->pProcessStateInfo->IsKernelServiceComplete())
			{
        pPAS->pProcessStateInfo->KernelServiceComplete(false);
        pPAS->status = RUN ;
			}
			else
				return ;
		}
		break ;

	  case RUN:
	    break ;

	  default:
		  return ;
	}

	Load(*pPAS) ;

	PIT_SetContextSwitch(false) ;

	KERNEL_MODE = false ;
	/* Switch Instruction */
	__asm__ __volatile__("lcall $0x28, $0x00") ;
	KERNEL_MODE = true ;

  Store(*pPAS) ;

  if(PIT_IsContextSwitch() == false || pPAS->status == TERMINATED) {
    Destroy(*pPAS);
  }

	if(debug_point) TRACE_LINE ;
	//IrqManager::Instance().EnableIRQ(TIMER_IRQ) ;
	EnableTaskSwitch();
	if(debug_point) TRACE_LINE ;
}

void ProcessManager::StartScheduler() {
  _currentProcessIt = _processMap.end();
	while(!_processMap.empty()) {
	  if (_currentProcessIt == _processMap.end()) {
      _currentProcessIt = _processMap.begin();
	  }
    _currentProcessID = _currentProcessIt->first;
    Process& process = *_currentProcessIt->second;

		DoContextSwitch(process) ;

    auto oldProcessIt = _currentProcessIt++;
    if (process.status == RELEASED) {
      delete &process;
      _processMap.erase(oldProcessIt);
    }
	}
}

void ProcessManager::EnableTaskSwitch()
{
	PIT_SetTaskSwitch(true) ;
}

void ProcessManager::DisableTaskSwitch()
{
	PIT_SetTaskSwitch(false) ;
}

void ProcessManager::Sleep(__volatile__ unsigned uiSleepTime) // in Mili Seconds
{
	if(DoPollWait())
	{
		KernelUtil::Wait(uiSleepTime) ;
		return ;
	}

	ProcessManager::DisableTaskSwitch() ;

  GetCurrentPAS().pProcessStateInfo->SleepTime(PIT_GetClockCount() + PIT_RoundSleepTime(uiSleepTime));
	GetCurrentPAS().status = WAIT_SLEEP ;

	ProcessManager_Yield() ;
}

void ProcessManager::WaitOnInterrupt(const IRQ& irq)
{
	if(DoPollWait())
	{
		KernelUtil::WaitOnInterrupt(irq);
		return;
	}

	ProcessManager::DisableTaskSwitch();
	
  GetCurrentPAS().pProcessStateInfo->Irq(&irq);
	GetCurrentPAS().status = WAIT_INT;

	ProcessManager_Yield();
}

void ProcessManager::WaitForEvent()
{
  if(DoPollWait())
  {
    while(!IsEventCompleted(GetCurProcId()))
    {
      __asm__ __volatile__("nop") ;
      __asm__ __volatile__("nop") ;
    }
    return;
  }

  ProcessManager::DisableTaskSwitch();
  GetCurrentPAS().status = WAIT_EVENT;

  ProcessManager_Yield();
}

void ProcessManager::WaitOnChild(int iChildProcessID)
{
	if(GetCurProcId() < 0)
		return ;

	if(iChildProcessID < 0 || iChildProcessID >= MAX_NO_PROCESS)
		return ;

	ProcessManager::DisableTaskSwitch() ;
	
  GetCurrentPAS().pProcessStateInfo->WaitChildProcId(iChildProcessID);
	GetCurrentPAS().status = WAIT_CHILD ;

	ProcessManager_Yield() ;
}

void ProcessManager::WaitOnResource(RESOURCE_KEYS resourceKey)
{
	if(GetCurProcId() < 0)
		return ;

	//ProcessManager_EnableTaskSwitch() ;
	ProcessManager::DisableTaskSwitch() ;
	
  GetCurrentPAS().pProcessStateInfo->WaitResourceId(resourceKey);
	GetCurrentPAS().status = WAIT_RESOURCE ;

	ProcessManager_Yield() ;
}

bool ProcessManager::IsChildAlive(int iChildProcessID) {
	if(iChildProcessID < 0 || iChildProcessID >= MAX_NO_PROCESS)
    return false;
	auto process = GetAddressSpace(iChildProcessID);
  return !process.isEmpty() && process.value().iParentProcessID == ProcessManager::GetCurrentProcessID();
}

byte ProcessManager::CreateKernelImage(const unsigned uiTaskAddress, int iParentProcessID,
                                       byte bIsFGProcess, unsigned uiParam1, unsigned uiParam2,
                                       int* iProcessID, const char* szKernelProcName)
{
  try {
    upan::uniq_ptr<Process> newPAS(GetNewPAS());
    auto parentProcess = GetAddressSpace(iParentProcessID);
    if(!parentProcess.isEmpty()) {
      newPAS->iDriveID = parentProcess.value().iDriveID ;

      MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(parentProcess.value().processPWD),
                         MemUtil_GetDS(), (unsigned)&(parentProcess.value().processPWD),
                         sizeof(FileSystem::PresentWorkingDirectory)) ;
    } else {
      int iDriveID = ROOT_DRIVE_ID ;
      newPAS->iDriveID = iDriveID ;

      if(iDriveID != CURRENT_DRIVE) {
        DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, false).goodValueOrThrow(XLOC);
        if(pDiskDrive->Mounted())
          newPAS->processPWD = pDiskDrive->_fileSystem.FSpwd;
      }
    }

    unsigned uiStackAddress ;
    RETURN_X_IF_NOT(ProcessAllocator_AllocateAddressSpaceForKernel(newPAS.get(), &uiStackAddress), ProcessAllocator_SUCCESS, ProcessManager_FAILURE) ;

    newPAS->bIsKernelProcess = true ;
    ProcessEnv_InitializeForKernelProcess() ;
  //	DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(GetAddressSpace(iNewProcessID).iDriveID) ;
  //
  //	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pDiskDrive->FSMountInfo.FSpwd),
  //	MemUtil_GetDS(), (unsigned)&GetAddressSpace(iNewProcessID).processPWD, sizeof(FileSystem::PresentWorkingDirectory)) ;

    unsigned uiStackTop = uiStackAddress - GLOBAL_DATA_SEGMENT_BASE + (PROCESS_KERNEL_STACK_PAGES * PAGE_SIZE) - 1 ;
    BuildKernelTaskState(uiTaskAddress, newPAS->taskState, uiStackTop, uiParam1, uiParam2) ;
    newPAS->processLDT.BuildForKernel();
    newPAS->iUserID = ROOT_USER_ID ;
    newPAS->iParentProcessID = iParentProcessID ;

    if(parentProcess.isEmpty())
      newPAS->_processGroup = new ProcessGroup(bIsFGProcess);
    else
      newPAS->_processGroup = parentProcess.value()._processGroup;

    newPAS->_processGroup->AddProcess();
    newPAS->_mainThreadID = newPAS->_processID;
    *iProcessID = newPAS->_processID;
    if(bIsFGProcess)
      newPAS->_processGroup->PutOnFGProcessList(newPAS->_processID);

    if(szKernelProcName == NULL)
      szKernelProcName = DEF_KERNEL_PROC_NAME ;

    newPAS->pname = (char*)DMM_AllocateForKernel(strlen(szKernelProcName) + 1);
    strcpy(newPAS->pname, szKernelProcName) ;

    newPAS->pProcessStateInfo = new ProcessStateInfo();
    newPAS->status = RUN ;

    AddToSchedulerList(*newPAS.release());
  } catch(upan::exception& ex) {
    ex.Print();
    return ProcessManager_FAILURE;
  }

	return ProcessManager_SUCCESS ;
}

byte ProcessManager::Create(const char* szProcessName, int iParentProcessID, byte bIsFGProcess, int* iProcessID, int iUserID, int iNumberOfParameters, char** szArgumentList)
{
  try {
    upan::uniq_ptr<Process> newPAS(GetNewPAS());

    unsigned uiEntryAdddress;
    unsigned uiProcessEntryStackSize;
    unsigned uiPDEAddress;

    newPAS->Load(szProcessName, &uiPDEAddress, &uiEntryAdddress, &uiProcessEntryStackSize, iNumberOfParameters, szArgumentList);
    auto parentProcess = GetAddressSpace(iParentProcessID);

    if(!parentProcess.isEmpty()) {
      newPAS->iDriveID = parentProcess.value().iDriveID ;
      newPAS->processPWD = parentProcess.value().processPWD;
    } else {
      int iDriveID = ROOT_DRIVE_ID ;
      newPAS->iDriveID = iDriveID ;

      if(iDriveID != CURRENT_DRIVE)
      {
        DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, false).goodValueOrThrow(XLOC);
        if(pDiskDrive->Mounted())
          newPAS->processPWD = pDiskDrive->_fileSystem.FSpwd;
      }
    }

    RETURN_X_IF_NOT(ProcessEnv_Initialize(uiPDEAddress, iParentProcessID), ProcessEnv_SUCCESS, ProcessManager_FAILURE) ;

    RETURN_X_IF_NOT(ProcFileManager_Initialize(uiPDEAddress, iParentProcessID), ProcessEnv_SUCCESS, ProcessManager_FAILURE) ;

    newPAS->bIsKernelProcess = false ;
    newPAS->_mainThreadID = newPAS->_processID;
    newPAS->uiNoOfPagesForDLLPTE = 0 ;
    newPAS->processLDT.Build();
    const uint32_t stackTopAddress = PROCESS_STACK_TOP_ADDRESS - PROCESS_BASE;
    newPAS->taskState.Build(stackTopAddress, uiPDEAddress, uiEntryAdddress, uiProcessEntryStackSize);

    newPAS->iUserID = iUserID == DERIVE_FROM_PARENT ? GetCurrentPAS().iUserID : iUserID;

    *iProcessID = newPAS->_processID;

    newPAS->iParentProcessID = iParentProcessID;

    if(parentProcess.isEmpty()) {
      newPAS->_processGroup = new ProcessGroup(bIsFGProcess);
    } else {
      newPAS->iDriveID = parentProcess.value().iDriveID ;
      newPAS->_processGroup = parentProcess.value()._processGroup;
    }

    newPAS->_processGroup->AddProcess();

    if(bIsFGProcess)
      newPAS->_processGroup->PutOnFGProcessList(newPAS->_processID);

    newPAS->pname = (char*)DMM_AllocateForKernel(strlen(szProcessName) + 1) ;
    strcpy(newPAS->pname, szProcessName) ;

    // Init Process State Info
    newPAS->pProcessStateInfo = new ProcessStateInfo();
    newPAS->status = RUN ;

    AddToSchedulerList(*newPAS.release());

    //MemManager::Instance().DisplayNoOfFreePages() ;
    return ProcessManager_SUCCESS ;
  }
  catch(const upan::exception& e)
  {
    e.Print();
    return ProcessManager_FAILURE;
  }
}

//TODO:
//1: Lock Env Page access
//2: Lock FileDescriptor Table access
//3: Lock process heap access
byte ProcessManager::CreateThreadTask(int parentID, unsigned threadEntryAddress, int iNumberOfParameters, char** szArgumentList, int& threadID) {
  try {
//    threadID = FindFreePAS();
//    Process& threadPAS = GetAddressSpace(threadID);
//    memset(&threadPAS, 0, sizeof(Process));
//
//    Process& parentPAS = GetAddressSpace(parentID)
//    unsigned uiProcessEntryStackSize;
//    unsigned uiPDEAddress =
//
//    newPAS.load(szProcessName, &uiPDEAddress, &uiEntryAdddress, &uiProcessEntryStackSize, iNumberOfParameters, szArgumentList);
//
//    newPAS.processPWD = parentPAS.processPWD;
//
//    threadPAS.bIsKernelProcess = false ;
//    threadPAS.uiNoOfPagesForDLLPTE = parentPAS.uiNoOfPagesForDLLPTE;
//    threadPAS.processLDT.Build();
//    threadPAS.taskState.Build(newPAS._processBase + newPAS._noOfPagesForProcess * PAGE_SIZE, uiPDEAddress, uiEntryAdddress, uiProcessEntryStackSize);
//
//    threadPAS.iUserID = parentPAS.iUserID;
//
//    threadPAS.iParentProcessID = parentPAS.iParentProcessID;
//    threadPAS._processID = threadID;
//    threadPAS._mainThreadID = parentPAS._mainThreadID;
//    newPAS.iDriveID = parentPAS.iDriveID;
//    newPAS._processGroup = parentPAS._processGroup;
//    newPAS._processGroup->AddProcess();
//
//    newPAS.pname = (char*)DMM_AllocateForKernel(strlen(szProcessName) + 1 + 12);
//    strcpy(newPAS.pname, szProcessName);
//    strcat(newPAS.pname, "_T");
//    strcat(newPAS.pname, upan::string::to_string(threadID).c_str());
//
//    // Init Process State Info
//    newPAS.pProcessStateInfo = new ProcessStateInfo();
//    newPAS.status = RUN ;
//
//    AddToSchedulerList(newPAS) ;
//
//    //MemManager::Instance().DisplayNoOfFreePages() ;
    return ProcessManager_SUCCESS ;
  } catch(const upan::exception& e) {
    e.Print();
    return ProcessManager_FAILURE;
  }
}

PS* ProcessManager::GetProcList(unsigned& uiListSize)
{
  ProcessSwitchLock lock;
  uiListSize = _processMap.size();

  PS* pProcList;
  PS* pPS;
  Process& pAddrSpc = GetCurrentPAS() ;

  if(pAddrSpc.bIsKernelProcess == true)
  {
    pPS = pProcList = (PS*)DMM_AllocateForKernel(sizeof(PS) * uiListSize) ;
  }
  else
  {
    pProcList = (PS*)DMM_Allocate(&pAddrSpc, sizeof(PS) * uiListSize) ;
    pPS = (PS*)(((unsigned)pProcList + PROCESS_BASE) - GLOBAL_DATA_SEGMENT_BASE) ;
  }

  auto it = _processMap.begin();
  for(int i = 0; it != _processMap.end(); ++i, ++it)
  {
    Process& p = *it->second;

    pPS[i].pid = p._processID;
    pPS[i].status = p.status ;
    pPS[i].iParentProcessID = p.iParentProcessID ;
    pPS[i].iProcessGroupID = p._processGroup->Id();
    pPS[i].iUserID = p.iUserID ;

    char* pname ;
    if(pAddrSpc.bIsKernelProcess == true)
    {
      pname = pPS[i].pname = (char*)DMM_AllocateForKernel(strlen(p.pname) + 1) ;
    }
    else
    {
      pPS[i].pname = (char*)DMM_Allocate(&pAddrSpc, strlen(p.pname) + 1) ;
      pname = (char*)((unsigned)pPS[i].pname + PROCESS_BASE - GLOBAL_DATA_SEGMENT_BASE) ;
    }
    strcpy(pname, p.pname) ;
  }

  return pProcList;
}

void ProcessManager::FreeProcListMem(PS* pProcList, unsigned uiListSize)
{
  Process& pAddrSpc = GetCurrentPAS();

	for(unsigned i = 0; i < uiListSize; i++)
	{
		if(pAddrSpc.bIsKernelProcess == true)
			DMM_DeAllocateForKernel((unsigned)pProcList[i].pname) ;
		else
			DMM_DeAllocate(&pAddrSpc, (unsigned)pProcList[i].pname) ;
	}
	
	if(pAddrSpc.bIsKernelProcess == true)
		DMM_DeAllocateForKernel((unsigned)pProcList) ;
	else
		DMM_DeAllocate(&pAddrSpc, (unsigned)pProcList) ;
}

void ProcessManager::SetDMMFlag(int iProcessID, bool flag) {
	GetAddressSpace(iProcessID).value().uiDMMFlag = flag;
}

bool ProcessManager::IsDMMOn(int iProcessID) {
	return GetAddressSpace(iProcessID).value().uiDMMFlag;
}

bool ProcessManager::IsKernelProcess(int iProcessID) {
	return GetAddressSpace(iProcessID).value().bIsKernelProcess ;
}


void ProcessManager_Exit()
{
  ProcessManager::DisableTaskSwitch();
	PIT_SetContextSwitch(false);
	ProcessManager_EXIT();
}

bool ProcessManager::IsResourceBusy(__volatile__ RESOURCE_KEYS uiType)
{
	return _resourceList[ uiType ] ;
}

void ProcessManager::SetResourceBusy(RESOURCE_KEYS uiType, bool bVal)
{
	_resourceList[ uiType ] = bVal ;
}

int ProcessManager::GetCurProcId()
{
	return KERNEL_MODE ? NO_PROCESS_ID : ProcessManager::GetCurrentProcessID();
}

void ProcessManager::Kill(int iProcessID) {
  GetAddressSpace(iProcessID).ifPresent([](Process& process) {
    Atomic::Swap((__volatile__ uint32_t &) (process.status), static_cast<int>(TERMINATED));
    ProcessManager_Yield();
  });
}

void ProcessManager::WakeUpFromKSWait(int iProcessID) {
  GetAddressSpace(iProcessID).ifPresent([](Process& process) {
    process.pProcessStateInfo->KernelServiceComplete(true);
  });
}

void ProcessManager::WaitOnKernelService() {
	if(GetCurProcId() < 0)
		return ; 

	ProcessManager::DisableTaskSwitch() ;

  GetCurrentPAS().pProcessStateInfo->KernelServiceComplete(false);
	GetCurrentPAS().status = WAIT_KERNEL_SERVICE ;

	ProcessManager_Yield() ;
}

bool ProcessManager::CopyDiskDrive(int iProcessID, int& iOldDriveId, FileSystem::PresentWorkingDirectory& mOldPWD) {
	if(GetCurProcId() < 0)
		return false;

  Process* pSrcPAS = &GetAddressSpace( iProcessID ).value();
  Process* pDestPAS = &GetCurrentPAS();

	iOldDriveId = pDestPAS->iDriveID ;
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pDestPAS->processPWD),
                    MemUtil_GetDS(), (unsigned)&(mOldPWD),
                    sizeof(FileSystem::PresentWorkingDirectory)) ;

  pDestPAS->iDriveID = pSrcPAS->iDriveID ;
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pSrcPAS->processPWD),
                    MemUtil_GetDS(), (unsigned)&(pDestPAS->processPWD),
                    sizeof(FileSystem::PresentWorkingDirectory)) ;

	return true;
}

bool ProcessManager::WakeupProcessOnInterrupt(Process& p)
{
  const IRQ& irq = *p.pProcessStateInfo->Irq();

	if(irq == StdIRQ::Instance().NO_IRQ)
		return true ;

	if(irq == StdIRQ::Instance().KEYBOARD_IRQ) {
    if(!(p._processGroup->IsFGProcess(p._processID) && p._processGroup->IsFGProcessGroup()))
			return false;
	}
	
	if(irq.Consume()) {
		p.status = RUN;
		return true;
	}

	return false;
}

void ProcessManager::DeAllocateResources(Process& pas) {
	if(pas.bIsKernelProcess) {
    ProcessAllocator_DeAllocateAddressSpaceForKernel(&pas);
//	} else if (pas.isChildThread()){
//    // Deallocate Stack
	} else {
		DLLLoader_DeAllocateProcessDLLPTEPages(&pas) ;
		DynamicLinkLoader_UnInitialize(&pas) ;

		DMM_DeAllocatePhysicalPages(&pas) ;

		ProcessEnv_UnInitialize(pas) ;
		ProcFileManager_UnInitialize(&pas);

    pas.DeAllocateAddressSpace();
	}
}

void ProcessManager::Release(Process& process)
{
  ProcessSwitchLock pLock;
	DMM_DeAllocateForKernel((unsigned)(process.pname)) ;
	if(process.pProcessStateInfo)
    delete process.pProcessStateInfo;
	process.status = RELEASED;
}

bool ProcessManager::DoPollWait() {
	return (KERNEL_MODE || !PIT_IsTaskSwitch()) ;
}

bool ProcessManager::ConditionalWait(const volatile unsigned* registry, unsigned bitPos, bool waitfor)
{
	if(bitPos > 31 || bitPos < 0)
		return false ;
  unsigned value = 1 << bitPos;

	int iMaxLimit = 1000 ; // 1 Sec
	unsigned uiSleepTime = 10 ; // 10 ms

	while(iMaxLimit > 10)
	{
    const bool res = ((*registry) & value) ? true : false;
    if(res == waitfor)
      return true;
		Sleep(uiSleepTime) ;
		iMaxLimit -= uiSleepTime ;
	}

	return false ;
}

bool ProcessManager::IsEventCompleted(int pid) {
  return GetProcessStateInfo(pid).IsEventCompleted();
}

void ProcessManager::EventCompleted(int pid) {
  GetProcessStateInfo(pid).EventCompleted();
}

ProcessStateInfo& ProcessManager::GetProcessStateInfo(int pid) {
  return GetAddressSpace(pid).map<ProcessStateInfo&>(
      [](Process& p) -> ProcessStateInfo& { return *p.pProcessStateInfo; })
      .valueOrElse(_kernelModeStateInfo);
}
