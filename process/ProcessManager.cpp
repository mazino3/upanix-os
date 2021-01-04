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
#include <DynamicLinkLoader.h>
#include <DMM.h>
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
#include <KernelProcess.h>
#include <UserThread.h>

int ProcessManager::_currentProcessID = NO_PROCESS_ID;
int ProcessManager::_upanixKernelProcessID = NO_PROCESS_ID;

static void ProcessManager_Yield()
{
	PIT_SetContextSwitch(true) ;
	ProcessManager_EXIT() ;
	ProcessManager_RESTORE() ;	
}

ProcessManager::ProcessManager() {
  for(int i = 0; i < MAX_RESOURCE; i++)
    _resourceList[i] = false ;

  PIT_SetContextSwitch(false) ;

	TaskState* sysTSS = (TaskState*)(MEM_SYS_TSS_START - GLOBAL_DATA_SEGMENT_BASE) ;
  memset(sysTSS, 0, sizeof(TaskState));
	sysTSS->CR3_PDBR = MEM_PDBR ;
	sysTSS->DEBUG_T_BIT = 0 ;
	sysTSS->IO_MAP_BASE = 103 ;

  ProcessLoader::Instance();

	KC::MDisplay().LoadMessage("Process Manager Initialization", Success);
}

UserProcess& ProcessManager::GetThreadParentProcess(int pid) {
  ProcessSwitchLock switchLock;
  auto parent = GetAddressSpace(pid);
  if (parent.isEmpty()) {
    throw upan::exception(XLOC, "No parent process found for pid: %d", pid);
  }

  UserProcess* userProcess = dynamic_cast<UserProcess*>(&parent.value());
  if (userProcess) {
    return *userProcess;
  }

  UserThread* userThread = dynamic_cast<UserThread*>(&parent.value());
  if (userThread) {
    return userThread->threadParent();
  }

  throw upan::exception(XLOC, "parent of a thread can be either a UserProcess or a UserThread");
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
  return GetAddressSpace(_currentProcessID).value();
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

void ProcessManager::AddToSchedulerList(Process& process) {
  ProcessSwitchLock switchLock;
  AddToProcessMap(process);
  process.setStatus(RUN);
  _processSchedulerList.push_back(&process);
}

void ProcessManager::AddToProcessMap(Process& process) {
  ProcessSwitchLock switchLock;
  if(_processMap.size() + 1 > MAX_NO_PROCESS)
    throw upan::exception(XLOC, "Max process limit reached!");
  _processMap.insert(ProcessMap::value_type(process.processID(), &process));
}

void ProcessManager::RemoveFromProcessMap(Process& process) {
  ProcessSwitchLock switchLock;
  _processMap.erase(process.processID());
}

void ProcessManager::DoContextSwitch(Process& process) {
  _currentProcessID = process.processID();
  ProcessStateInfo& stateInfo = process.stateInfo();

	switch(process.status()) {
	  case RELEASED:
	    return;
	  case TERMINATED:
	    if (process.parentProcessID() == UpanixKernelProcessID()) {
	      process.Release();
	    }
	    return;
  	case WAIT_SLEEP:
		{
      if(PIT_GetClockCount() >= stateInfo.SleepTime())
			{
        stateInfo.SleepTime(0) ;
				process.setStatus(RUN);
			}
			else
			{
				if(debug_point) 
				{
          printf("\n Sleep Time: %u", stateInfo.SleepTime()) ;
					printf("\n PIT Tick Count: %u", PIT_GetClockCount()) ;
					printf("\n") ;
				}
				return ;
			}
		}
		break ;

	  case WAIT_INT:
		{
			if(!WakeupProcessOnInterrupt(process))
				return ;
		}
		break ;

    case WAIT_EVENT:
    {
      if(!IsEventCompleted(process.processID()))
        return;
      process.setStatus(RUN);
    }
    break;
	
	  case WAIT_CHILD:
		{
      if(stateInfo.WaitChildProcId() < 0)
			{
        stateInfo.WaitChildProcId(NO_PROCESS_ID);
				process.setStatus(RUN);
			}
			else
			{
			  auto childProcess = GetAddressSpace(stateInfo.WaitChildProcId());
				if(childProcess.isEmpty() || childProcess.value().parentProcessID() != _currentProcessID)
				{
          process.removeChildProcessID(stateInfo.WaitChildProcId());
          stateInfo.WaitChildProcId(NO_PROCESS_ID);
					process.setStatus(RUN);
				}
				else if(childProcess.value().status() == TERMINATED && childProcess.value().parentProcessID() == _currentProcessID)
				{
          childProcess.value().Release();
          process.removeChildProcessID(stateInfo.WaitChildProcId());
          stateInfo.WaitChildProcId(NO_PROCESS_ID);
					process.setStatus(RUN);
				}
				else
					return ;
			}
		}
		break ;

	  case WAIT_RESOURCE:
		{
      if(stateInfo.WaitResourceId() == RESOURCE_NIL)
			{
				process.setStatus(RUN);
			}
			else
			{
        if(_resourceList[stateInfo.WaitResourceId()] == false)
				{ 
          stateInfo.WaitResourceId(RESOURCE_NIL);
					process.setStatus(RUN);
				}
				else
					return ;
			}
		}
		break ;

	  case WAIT_KERNEL_SERVICE:
		{
      if(stateInfo.IsKernelServiceComplete())
			{
        stateInfo.KernelServiceComplete(false);
        process.setStatus(RUN);
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

	process.Load();

	PIT_SetContextSwitch(false) ;

	KERNEL_MODE = false ;
	/* Switch Instruction */
	__asm__ __volatile__("lcall $0x28, $0x00") ;
	KERNEL_MODE = true ;

	process.Store();

  if(PIT_IsContextSwitch() == false || process.status() == TERMINATED) {
    process.Destroy();
  }

	if(debug_point) TRACE_LINE ;
	//IrqManager::Instance().EnableIRQ(TIMER_IRQ) ;
	EnableTaskSwitch();
	if(debug_point) TRACE_LINE ;
}

void ProcessManager::StartScheduler() {
  auto it = _processSchedulerList.begin();
	while(!_processSchedulerList.empty()) {
	  if (it == _processSchedulerList.end()) {
	    it = _processSchedulerList.begin();
	  }
    Process& process = (*it)->forSchedule();
		DoContextSwitch(process);

    auto oldIt = it++;
    if (!process.isChildThread() && process.status() == RELEASED) {
      _processSchedulerList.erase(oldIt);
      RemoveFromProcessMap(process);
      delete &process;
    }
	}
}

void ProcessManager::EnableTaskSwitch() {
	PIT_EnableTaskSwitch();
}

void ProcessManager::DisableTaskSwitch() {
  PIT_DisableTaskSwitch();
}

void ProcessManager::Sleep(__volatile__ unsigned uiSleepTime) // in Mili Seconds
{
	if(DoPollWait())
	{
		KernelUtil::Wait(uiSleepTime) ;
		return ;
	}

	ProcessManager::DisableTaskSwitch() ;

  GetCurrentPAS().stateInfo().SleepTime(PIT_GetClockCount() + PIT_RoundSleepTime(uiSleepTime));
	GetCurrentPAS().setStatus(WAIT_SLEEP);

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
	
  GetCurrentPAS().stateInfo().Irq(&irq);
	GetCurrentPAS().setStatus(WAIT_INT);

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
  GetCurrentPAS().setStatus(WAIT_EVENT);

  ProcessManager_Yield();
}

void ProcessManager::WaitOnChild(int iChildProcessID)
{
	if(GetCurProcId() < 0)
		return ;

	if(iChildProcessID < 0 || iChildProcessID >= MAX_NO_PROCESS)
		return ;

	ProcessManager::DisableTaskSwitch() ;
	
  GetCurrentPAS().stateInfo().WaitChildProcId(iChildProcessID);
	GetCurrentPAS().setStatus(WAIT_CHILD);

	ProcessManager_Yield() ;
}

void ProcessManager::WaitOnResource(RESOURCE_KEYS resourceKey)
{
	if(GetCurProcId() < 0)
		return ;

	//ProcessManager_EnableTaskSwitch() ;
	ProcessManager::DisableTaskSwitch() ;
	
  GetCurrentPAS().stateInfo().WaitResourceId(resourceKey);
	GetCurrentPAS().setStatus(WAIT_RESOURCE);

	ProcessManager_Yield() ;
}

bool ProcessManager::IsChildAlive(int iChildProcessID) {
	if(iChildProcessID < 0 || iChildProcessID >= MAX_NO_PROCESS)
    return false;
	auto process = GetAddressSpace(iChildProcessID);
  return !process.isEmpty() && process.value().parentProcessID() == ProcessManager::GetCurrentProcessID();
}

int ProcessManager::CreateKernelProcess(const upan::string& name, const unsigned uiTaskAddress, int iParentProcessID,
                                        byte bIsFGProcess, unsigned uiParam1, unsigned uiParam2) {
  try {
    upan::uniq_ptr<Process> newPAS(new KernelProcess(name, uiTaskAddress, iParentProcessID, bIsFGProcess, uiParam1, uiParam2));
    int pid = newPAS->processID();
    AddToSchedulerList(*newPAS.release());
    return pid;
  } catch(upan::exception& ex) {
    ex.Print();
  }
	return -1;
}

int ProcessManager::Create(const upan::string& name, int iParentProcessID, byte bIsFGProcess, int iUserID, int iNumberOfParameters, char** szArgumentList) {
  try {
    upan::uniq_ptr<Process> newPAS(new UserProcess(name, iParentProcessID, iUserID, bIsFGProcess, iNumberOfParameters, szArgumentList));
    int pid = newPAS->processID();
    AddToSchedulerList(*newPAS.release());
    //MemManager::Instance().DisplayNoOfFreePages() ;
    return pid;
  }
  catch(const upan::exception& e) {
    e.Print();
  }
  return -1;
}

//TODO:
//1: Lock Env Page access
//2: Lock FileDescriptor Table access
//3: Lock process heap access
//4: DLL service
int ProcessManager::CreateThreadTask(int parentID, uint32_t threadCaller, uint32_t threadEntryAddress, void* arg) {
  try {
    UserProcess& parent = ProcessManager::Instance().GetThreadParentProcess(parentID);
    upan::uniq_ptr<Process> threadPAS(new UserThread(parent, threadCaller, threadEntryAddress, arg));
    int threadID = threadPAS->processID();
    AddToProcessMap(*threadPAS.release());
    //MemManager::Instance().DisplayNoOfFreePages() ;
    return threadID;
  } catch(const upan::exception& e) {
    e.Print();
  }
  return -1;
}

PS* ProcessManager::GetProcList(unsigned& uiListSize)
{
  ProcessSwitchLock lock;
  uiListSize = _processMap.size();

  PS* pProcList;
  PS* pPS;
  Process& pAddrSpc = GetCurrentPAS() ;

  if(pAddrSpc.isKernelProcess())
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

    pPS[i].pid = p.processID();
    pPS[i].status = p.status() ;
    pPS[i].iParentProcessID = p.parentProcessID() ;
    pPS[i].iProcessGroupID = p.processGroup()->Id();
    pPS[i].iUserID = p.userID() ;

    char* pname ;
    if(pAddrSpc.isKernelProcess())
    {
      pname = pPS[i].pname = (char*)DMM_AllocateForKernel(p.name().length() + 1) ;
    }
    else
    {
      pPS[i].pname = (char*)DMM_Allocate(&pAddrSpc, p.name().length() + 1) ;
      pname = (char*)((unsigned)pPS[i].pname + PROCESS_BASE - GLOBAL_DATA_SEGMENT_BASE) ;
    }
    strcpy(pname, p.name().c_str()) ;
  }

  return pProcList;
}

void ProcessManager::FreeProcListMem(PS* pProcList, unsigned uiListSize)
{
  Process& pAddrSpc = GetCurrentPAS();

	for(unsigned i = 0; i < uiListSize; i++)
	{
		if(pAddrSpc.isKernelProcess())
			DMM_DeAllocateForKernel((unsigned)pProcList[i].pname) ;
		else
			DMM_DeAllocate(&pAddrSpc, (unsigned)pProcList[i].pname) ;
	}
	
	if(pAddrSpc.isKernelProcess())
		DMM_DeAllocateForKernel((unsigned)pProcList) ;
	else
		DMM_DeAllocate(&pAddrSpc, (unsigned)pProcList) ;
}

void ProcessManager::SetDMMFlag(int iProcessID, bool flag) {
	GetAddressSpace(iProcessID).value().setDmmFlag(flag);
}

bool ProcessManager::IsDMMOn(int iProcessID) {
	return GetAddressSpace(iProcessID).value().isDmmFlag();
}

bool ProcessManager::IsKernelProcess(int iProcessID) {
	return GetAddressSpace(iProcessID).value().isKernelProcess();
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
    process.setStatus(TERMINATED);
    ProcessManager_Yield();
  });
}

void ProcessManager::WakeUpFromKSWait(int iProcessID) {
  GetAddressSpace(iProcessID).ifPresent([](Process& process) {
    process.stateInfo().KernelServiceComplete(true);
  });
}

void ProcessManager::WaitOnKernelService() {
	if(GetCurProcId() < 0)
		return ; 

	ProcessManager::DisableTaskSwitch() ;

  GetCurrentPAS().stateInfo().KernelServiceComplete(false);
	GetCurrentPAS().setStatus(WAIT_KERNEL_SERVICE);

	ProcessManager_Yield() ;
}

bool ProcessManager::CopyDiskDrive(int iProcessID, int& iOldDriveId, FileSystem::PresentWorkingDirectory& mOldPWD) {
	if(GetCurProcId() < 0)
		return false;

  Process* pSrcPAS = &GetAddressSpace( iProcessID ).value();
  Process* pDestPAS = &GetCurrentPAS();

	iOldDriveId = pDestPAS->driveID() ;
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pDestPAS->processPWD()),
                    MemUtil_GetDS(), (unsigned)&(mOldPWD),
                    sizeof(FileSystem::PresentWorkingDirectory)) ;

  pDestPAS->setDriveID(pSrcPAS->driveID());
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pSrcPAS->processPWD()),
                    MemUtil_GetDS(), (unsigned)&(pDestPAS->processPWD()),
                    sizeof(FileSystem::PresentWorkingDirectory)) ;

	return true;
}

bool ProcessManager::WakeupProcessOnInterrupt(Process& p)
{
  const IRQ& irq = *p.stateInfo().Irq();

	if(irq == StdIRQ::Instance().NO_IRQ)
		return true ;

	if(irq == StdIRQ::Instance().KEYBOARD_IRQ) {
    if(!(p.processGroup()->IsFGProcess(p.processID()) && p.processGroup()->IsFGProcessGroup()))
			return false;
	}
	
	if(irq.Consume()) {
		p.setStatus(RUN);
		return true;
	}

	return false;
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
      [](Process& p) -> ProcessStateInfo& { return p.stateInfo(); })
      .valueOrElse(_kernelModeStateInfo);
}
