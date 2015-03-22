/*
 *	Mother Operating System - An x86 based Operating System
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
#include <DisplayManager.h>
#include <MemUtil.h>
#include <MemManager.h>
#include <FileSystem.h>
#include <Directory.h>
#include <PIC.h>
#include <PIT.h>
#include <AsmUtil.h>
#include <ProcessLoader.h>
#include <ProcessAllocator.h>
#include <DynamicLinkLoader.h>
#include <DMM.h>
#include <DSUtil.h>
#include <DLLLoader.h>
#include <ProcessEnv.h>
#include <KernelService.h>
#include <ProcFileManager.h>
#include <UserManager.h>
#include <ProcessGroupManager.h>
#include <MountManager.h>
#include <String.h>
#include <MOSMain.h>
#include <ProcessConstants.h>
#include <KernelUtil.h>

/** Global ProcessManager Variables **/
ProcessAddressSpace *ProcessManager_processAddressSpace ;
int ProcessManager_iCurrentProcessID ;
/***********************************************************/

static const char* DEF_KERNEL_PROC_NAME = "Kernel Proc" ;

static DSUtil_SLL ProcessManager_ProcessList ;
static unsigned ProcessManager_uiCount ;

static unsigned ProcessManager_InterruptOccured[PIC::MAX_INTERRUPT] ;
static unsigned ProcessManager_ResourceList[MAX_RESOURCE] ;

#define MAX_PROC_ON_INT_QUEUE 100

static DSUtil_Queue ProcessManager_InterruptQueue[PIC::MAX_INTERRUPT] ;
static unsigned ProcessManager_InterruptQueueBuffer[PIC::MAX_INTERRUPT][MAX_PROC_ON_INT_QUEUE] ;

/*********************** Static Functions *****************************/
			
static bool ProcessManager_WakeupProcessOnInterrupt(__volatile__ int iProcessID)
{
	__volatile__ ProcessAddressSpace* p = &ProcessManager_processAddressSpace[iProcessID] ;
	const IRQ* pIRQ = p->pProcessStateInfo->pIRQ ;

	if(pIRQ == &PIC::NO_IRQ)
		return true ;

	if(pIRQ->GetIRQNo() == PIC::KEYBOARD_IRQ)
	{
		//if(!ProcessGroupManager_IsFGProcessGroup(p->iProcessGroupID)
		//|| !ProcessGroupManager_IsFGProcess(p->iProcessGroupID, iProcessID))
		if(! ( ProcessGroupManager_IsFGProcess(p->iProcessGroupID, iProcessID) && ProcessGroupManager_IsFGProcessGroup(p->iProcessGroupID) ) )
		{
			return false ;
		}
	}
	
	if(ProcessManager_GetFromInterruptQueue(pIRQ) == ProcessManager_SUCCESS)
	{
		p->status = RUN ;
		return true ;
	}

	return false ;
}

static void ProcessManager_InsertIntoProcessList(int iProcessID)
{
	ProcessManager_uiCount++ ;
	if(DSUtil_InsertSLL(&ProcessManager_ProcessList, iProcessID) != DSUtil_SUCCESS)
		KC::MDisplay().Message("\n Failed to Insert Into Process List Data Structure\n", ' ') ;
}

static void ProcessManager_DeleteFromProcessList(int iProcessID)
{
	ProcessManager_uiCount-- ;
	if(DSUtil_DeleteSLL(&ProcessManager_ProcessList, iProcessID) != DSUtil_SUCCESS)
		KC::MDisplay().Message("\n Failed to Delete From Process List Data Structure\n", ' ') ;
}

static void ProcessManager_DeAllocateProcessInitDockMem(ProcessAddressSpace* pPAS)
{
	unsigned uiPDEAddress = pPAS->taskState.CR3_PDBR ;
	unsigned uiPDEIndex = ((PROCESS_INIT_DOCK_ADDRESS >> 22) & 0x3FF) ;
	unsigned uiPTEIndex = ((PROCESS_INIT_DOCK_ADDRESS >> 12) & 0x3FF) ;

	// We are strongly assuming that this PTE and page are allocated solely for ProcInit docking
	// Meaning, we are not expecting any page faults to happen in this region of Process Strict Base of 16 MB
	// and actual process start address - which again is expected to fall beyond 20 MB page
	// In a way this is also controlled in KC::MMemManager().AllocatePage() by monitoring faulty address !
	
	unsigned uiPTEAddress = ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] & 0xFFFFF000 ;
	
	unsigned uiPageNumber = (((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] & 0xFFFFF000) / PAGE_SIZE ;
	
	KC::MMemManager().DeAllocatePhysicalPage(uiPageNumber) ;
	KC::MMemManager().DeAllocatePhysicalPage(uiPTEAddress/PAGE_SIZE) ;
}

static void ProcessManager_DeAllocateResources(int iProcessID)
{
	ProcessAddressSpace* pPAS = &ProcessManager_processAddressSpace[iProcessID] ;

	if(pPAS->bIsKernelProcess == false)
	{
		DLLLoader_DeAllocateProcessDLLPTEPages(pPAS) ;
		DynamicLinkLoader_UnInitialize(pPAS) ;

		DMM_DeAllocatePhysicalPages(pPAS) ;

		ProcessEnv_UnInitialize(pPAS) ;
		ProcFileManager_UnInitialize(pPAS) ;

		ProcessManager_DeAllocateProcessInitDockMem(pPAS) ;

		ProcessAllocator_DeAllocateAddressSpace(pPAS) ;
	}
	else
	{
		ProcessAllocator_DeAllocateAddressSpaceForKernel(pPAS) ;
	}
}

void ProcessManager_Release(int iProcessID)
{
	ProcessManager_DisableTaskSwitch() ;

	ProcessAddressSpace* pPAS = &ProcessManager_processAddressSpace[iProcessID] ;

	ProcessManager_DeleteFromSchedulerList(iProcessID) ;

	DMM_DeAllocateForKernel((unsigned)(pPAS->pname)) ;

	if(pPAS->pProcessStateInfo)
		DMM_DeAllocateForKernel((unsigned)(pPAS->pProcessStateInfo)) ;

	ProcessManager_EnableTaskSwitch() ;
}

static void ProcessManager_Yield()
{
	PIT_SetContextSwitch(true) ;
	ProcessManager_EXIT() ;
	ProcessManager_RESTORE() ;	
}

static bool ProcessManager_DoPoleWait()
{
	return (KERNEL_MODE || !PIT_IsTaskSwitch()) ;
}

/*********************************************************************/

void ProcessManager_Initialize()
{
	byte bStatus = SUCCESS ;

	if(((sizeof(ProcessAddressSpace) * MAX_NO_PROCESS)) > (MEM_PAS_END - MEM_PAS_START))
	{
		KC::MDisplay().Message("\n Process Address Space InSufficient\n", '\r') ;
		bStatus = FAILURE ;
	}
	else
	{
		ProcessManager_processAddressSpace = (ProcessAddressSpace*)MEM_PAS_START ;

		int i ;
		for(i = 0; i < MAX_NO_PROCESS; i++)
		{
			ProcessManager_processAddressSpace[i].bFree = true ;
			ProcessManager_processAddressSpace[i].status = NEW ;
		}
				
		for(i = 0; i < PIC::MAX_INTERRUPT; i++)
			ProcessManager_InterruptOccured[i] = false ;

		for(i = 0; i < MAX_RESOURCE; i++)
			ProcessManager_ResourceList[i] = false ;

		ProcessManager_iCurrentProcessID = NO_PROCESS_ID ;
		ProcessManager_uiCount = 0 ;
		PIT_SetContextSwitch(false) ;
	}

	TaskState* sysTSS = (TaskState*)(MEM_SYS_TSS_START - GLOBAL_DATA_SEGMENT_BASE) ;
	sysTSS->CR3_PDBR = MEM_PDBR ;
	sysTSS->DEBUG_T_BIT = 0 ;
	sysTSS->IO_MAP_BASE = 103 ;

	String_Copy(PROCESS_DLL_FILE, OSIN_PATH) ;
	String_CanCat(PROCESS_DLL_FILE, __PROCESS_DLL_FILE) ;

	String_Copy(PROCESS_START_UP_FILE, OSIN_PATH) ;
	String_CanCat(PROCESS_START_UP_FILE, __PROCESS_START_UP_FILE) ;

	if(ProcessGroupManager_Initialize() != ProcessGroupManager_SUCCESS)
		bStatus = FAILURE ;

	int i ;
	for(i = 0; i < PIC::MAX_INTERRUPT; i++)
		DSUtil_InitializeQueue(&ProcessManager_InterruptQueue[i], (unsigned)&(ProcessManager_InterruptQueueBuffer[i]), MAX_PROC_ON_INT_QUEUE) ;

	DLLLoader_Initialize() ;	

	DSUtil_InitializeSLL(&ProcessManager_ProcessList) ;

	ProcFileManager_InitForKernel();

	KC::MDisplay().LoadMessage("Process Manager Initialization", bStatus) ;
}

void ProcessManager_BuildCallGate(unsigned short usGateSelector, unsigned uiOffset, unsigned short usSelector, byte bParameterCount)
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

void ProcessManager_BuildIntGate(unsigned short usGateSelector, unsigned uiOffset, unsigned short usSelector, byte bParameterCount)
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

static byte ProcessManager_FindFreePAS(int& iNewProcessID)
{
	static Mutex m ;

	m.Lock() ;

	if(ProcessManager_uiCount + 1 > MAX_NO_PROCESS)
	{
		m.UnLock() ;
		return ProcessManager_ERR_MAX_PROCESS_EXCEEDED ;
	}

	int i ;
	for(i = 0; i < MAX_NO_PROCESS; i++)
	{
		if(ProcessManager_processAddressSpace[i].bFree == true)
		{
			ProcessManager_processAddressSpace[i].uiAUTAddress = NULL ;
			ProcessManager_processAddressSpace[i].uiDMMFlag = false ;
			ProcessManager_processAddressSpace[i].bFree = false ;
			ProcessManager_processAddressSpace[i].status = NEW ;
			
			iNewProcessID = i ;
			
			m.UnLock() ;
			return ProcessManager_SUCCESS ;
		}
	}

	m.UnLock() ;
	
	return ProcessManager_ERR_NO_FREE_PAS ;
}

void ProcessManager_BuildDescriptor(Descriptor* descriptor, unsigned uiLimit, unsigned uiBase, byte bType, byte bFlag)
{
	descriptor->usLimit15_0 = (0xFFFF & uiLimit) ;
	descriptor->usBase15_0 = (0xFFFF & uiBase) ;
	descriptor->bBase23_16 = (uiBase >> 16) & 0xFF ;
	descriptor->bType = bType ;
	descriptor->bLimit19_16_Flags = (bFlag << 4) | ((uiLimit >> 16) & 0xF) ;
	descriptor->bBase31_24 = (uiBase >> 24) & 0xFF ;
}

void ProcessManager_BuildLDT(ProcessLDT* processLDT)
{
	unsigned uiSegmentBase = PROCESS_BASE ;
	unsigned uiProcessLimit = 0xFFFFF ;

	ProcessManager_BuildDescriptor(&processLDT->NullDesc, 0, 0, 0, 0) ;
	ProcessManager_BuildDescriptor(&processLDT->LinearDesc, uiProcessLimit, uiSegmentBase, 0xF2, 0xC) ;
	ProcessManager_BuildDescriptor(&processLDT->CodeDesc, uiProcessLimit, uiSegmentBase, 0xFA, 0xC) ;
	ProcessManager_BuildDescriptor(&processLDT->DataDesc, uiProcessLimit, uiSegmentBase, 0xF2, 0xC) ;
	ProcessManager_BuildDescriptor(&processLDT->StackDesc, uiProcessLimit, uiSegmentBase, 0xF2, 0xC) ;
	
	ProcessManager_BuildDescriptor(&processLDT->CallGateStackDesc, uiProcessLimit, GLOBAL_DATA_SEGMENT_BASE, 0x92, 0xC) ;
}

void
ProcessManager_BuildKernelTaskLDT(int iProcessID)
{
	ProcessLDT* processLDT = &ProcessManager_processAddressSpace[iProcessID].processLDT ;

	ProcessManager_BuildDescriptor(&processLDT->NullDesc, 0, 0, 0, 0) ;
//	ProcessManager_BuildDescriptor(&processLDT->LinearDesc, 0xFFFFF, 0x00, 0x92, 0xC) ;
	ProcessManager_BuildDescriptor(&processLDT->LinearDesc, 0xFFFFF, GLOBAL_DATA_SEGMENT_BASE, 0x92, 0xC) ;
	ProcessManager_BuildDescriptor(&processLDT->CodeDesc, 0xFFFFF, GLOBAL_DATA_SEGMENT_BASE, 0x9A, 0xC) ;
	ProcessManager_BuildDescriptor(&processLDT->DataDesc, 0xFFFFF, GLOBAL_DATA_SEGMENT_BASE, 0x92, 0xC) ;
	ProcessManager_BuildDescriptor(&processLDT->StackDesc, 0xFFFFF, GLOBAL_DATA_SEGMENT_BASE, 0x92, 0xC) ;
}

void
ProcessManager_BuildIntTaskState(const unsigned uiTaskAddress, const unsigned uiTSSAddress, const int stack)
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

void
ProcessManager_BuildKernelTaskState(const unsigned uiTaskAddress, const int iProcessID, const unsigned uiStackTop, unsigned uiParam1, unsigned uiParam2)
{
	TaskState* taskState = &ProcessManager_processAddressSpace[iProcessID].taskState ;
	memset(taskState, 0, sizeof(TaskState)) ;

	taskState->EIP = uiTaskAddress ;

	((unsigned*)(uiStackTop - 8))[0] = uiParam1 ;
	((unsigned*)(uiStackTop - 8))[1] = uiParam2 ;
	taskState->ESP = uiStackTop - 12 ;

	taskState->ES = 0x8 | 0x4 ;

	taskState->CS = 0x10 | 0x4 ;

	taskState->DS = 0x18 | 0x4 ;
	taskState->FS = 0x18 | 0x4 ;
	taskState->GS = 0x18 | 0x4 ;

	taskState->SS = 0x20 | 0x4 ;

	taskState->LDT = 0x50 ;

	taskState->CR3_PDBR = MEM_PDBR ;
	taskState->EFLAGS = 0x202 ;
	taskState->DEBUG_T_BIT = 0x00 ;
	taskState->IO_MAP_BASE = 103 ; // > TSS Limit => No I/O Permission Bit Map present
}

void ProcessManager_BuildTaskState(ProcessAddressSpace* pProcessAddressSpace, unsigned uiPDEAddress, unsigned uiEntryAdddress, unsigned uiProcessEntryStackSize)
{
	int pr = 100 ;
	TaskState* taskState = &(pProcessAddressSpace->taskState) ;
	memset(taskState, 0, sizeof(TaskState)) ;

	taskState->CR3_PDBR = uiPDEAddress ;
	taskState->EIP = uiEntryAdddress ;
	taskState->ESP = pProcessAddressSpace->uiProcessBase + (pProcessAddressSpace->uiNoOfPagesForProcess - PROCESS_CG_STACK_PAGES) * PAGE_SIZE - uiProcessEntryStackSize ;

	if(pr == 1) //GDT - Low Priority
	{
		taskState->ES = 0x50 | 0x3 ;
	
		taskState->CS = 0x40 | 0x3 ; 
	
		taskState->DS = 0x48 | 0x3 ;
		taskState->SS = 0x48 | 0x3 ;
		taskState->FS = 0x48 | 0x3 ;
		taskState->GS = 0x48 | 0x3 ;
		taskState->LDT = 0x0 ;
	}
	else if(pr == 2) //GDT - High Priority
	{
		taskState->ES = 0x8 ;
	
		taskState->CS = 0x10 ;
	
		taskState->DS = 0x18 ;
		taskState->SS = 0x18 ;
		taskState->FS = 0x18 ;
		taskState->GS = 0x18 ;
		taskState->LDT = 0x0 ;
	}
	else //LDT
	{
		taskState->ES = 0x8 | 0x7 ;
	
		taskState->CS = 0x10 | 0x7 ;
	
		taskState->DS = 0x18 | 0x7 ;
		taskState->FS = 0x18 | 0x7 ;
		taskState->GS = 0x18 | 0x7 ;

		taskState->SS = 0x20 | 0x7 ;

		taskState->SS0 = 0x28 | 0x4 ;
		taskState->ESP0 = PROCESS_BASE + pProcessAddressSpace->uiProcessBase + (pProcessAddressSpace->uiNoOfPagesForProcess) * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE ;
		taskState->LDT = 0x50 ;
	}
	taskState->EFLAGS = 0x202 ;
	taskState->DEBUG_T_BIT = 0x00 ;
	taskState->IO_MAP_BASE = 103 ; // > TSS Limit => No I/O Permission Bit Map present
}

void ProcessManager_AddToSchedulerList(int iNewProcessID)
{
	ProcessManager_DisableTaskSwitch() ;

	if(ProcessManager_iCurrentProcessID == NO_PROCESS_ID)
	{
		ProcessManager_iCurrentProcessID = iNewProcessID ;
		ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].iNextProcessID = ProcessManager_iCurrentProcessID ;
		ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].iPrevProcessID = ProcessManager_iCurrentProcessID ;
	}
	else
	{
		int iPrevProcessID = ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].iPrevProcessID ;
		
		ProcessManager_processAddressSpace[iNewProcessID].iNextProcessID = ProcessManager_iCurrentProcessID ;
		ProcessManager_processAddressSpace[iNewProcessID].iPrevProcessID = iPrevProcessID ;

		ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].iPrevProcessID = iNewProcessID ;
		ProcessManager_processAddressSpace[iPrevProcessID].iNextProcessID = iNewProcessID ;
	}

	ProcessManager_InsertIntoProcessList(iNewProcessID) ;

	ProcessManager_EnableTaskSwitch() ;
}

void ProcessManager_DeleteFromSchedulerList(int iDeleteProcessID)
{
	if(ProcessManager_processAddressSpace[iDeleteProcessID].iNextProcessID == iDeleteProcessID)
	{
		ProcessManager_iCurrentProcessID = NO_PROCESS_ID ;
	}
	else
	{
		int iPrevProcessID = ProcessManager_processAddressSpace[iDeleteProcessID].iPrevProcessID ;
		int iNextProcessID = ProcessManager_processAddressSpace[iDeleteProcessID].iNextProcessID ;

		ProcessManager_processAddressSpace[iPrevProcessID].iNextProcessID = iNextProcessID ;
		ProcessManager_processAddressSpace[iNextProcessID].iPrevProcessID = iPrevProcessID ;
	}

	ProcessManager_DeleteFromProcessList(iDeleteProcessID) ;

	ProcessManager_processAddressSpace[iDeleteProcessID].bFree = true ;
}

void ProcessManager_Destroy(int iDeleteProcessID)
{
	// Release process
	ProcessAddressSpace* pPAS = &ProcessManager_processAddressSpace[iDeleteProcessID] ;

	Atomic::Swap((__volatile__ int&)(pPAS->status), static_cast<int>(TERMINATED)) ;

	//KC::MMemManager().DisplayNoOfFreePages();
	//KC::MMemManager().DisplayNoOfAllocPages(0,0);
	
	// Deallocate Resources
	ProcessManager_DeAllocateResources(iDeleteProcessID) ;

		// Release From Process Group
	ProcessGroupManager_RemoveFromFGProcessList(iDeleteProcessID) ;
	ProcessGroupManager_RemoveProcessFromGroup(iDeleteProcessID) ;

	int iProcessGroupID = pPAS->iProcessGroupID ;

	if(ProcessGroupManager_GetProcessCount(iProcessGroupID) == 0)
		ProcessGroupManager_DestroyProcessGroup(iProcessGroupID) ;

		// Child processes of this process (if any) will be redirected to "kernel" process
	int iNextProcessID = pPAS->iNextProcessID ;
	while(true)
	{
		if(iNextProcessID == NO_PROCESS_ID || iNextProcessID == iDeleteProcessID)
			break ;

		ProcessAddressSpace* pc = &ProcessManager_processAddressSpace[iNextProcessID] ;

		if(pc->iParentProcessID == iDeleteProcessID)
		{
			if(pc->status == TERMINATED)
			{
				ProcessManager_Release(iNextProcessID) ;
			}
			else
			{
				pc->iParentProcessID = MOSMain_KernelProcessID() ;
			}
		}

		iNextProcessID = pc->iNextProcessID ;
	}	

	int iParentProcessID = pPAS->iParentProcessID ;
	
	if(iParentProcessID == NO_PROCESS_ID)
		ProcessManager_Release(iDeleteProcessID) ;
	
	//KC::MMemManager().DisplayNoOfFreePages();
	//KC::MMemManager().DisplayNoOfAllocPages(0,0);
}

void ProcessManager_Load(int iProcessID)
{
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&ProcessManager_processAddressSpace[iProcessID].processLDT,
					SYS_LINEAR_SELECTOR_DEFINED, MEM_LDT_START, sizeof(ProcessLDT)) ;

	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&ProcessManager_processAddressSpace[iProcessID].taskState,
					SYS_LINEAR_SELECTOR_DEFINED, MEM_USER_TSS_START, sizeof(TaskState)) ;
}

void
ProcessManager_Store(int iProcessID)
{
	MemUtil_CopyMemory(SYS_LINEAR_SELECTOR_DEFINED, MEM_LDT_START, MemUtil_GetDS(), 
					(unsigned)&ProcessManager_processAddressSpace[iProcessID].processLDT, sizeof(ProcessLDT)) ;

	MemUtil_CopyMemory(SYS_LINEAR_SELECTOR_DEFINED, MEM_USER_TSS_START,	MemUtil_GetDS(), 
					(unsigned)&ProcessManager_processAddressSpace[iProcessID].taskState, sizeof(TaskState)) ;
}

void ProcessManager_DoContextSwitch(int iProcessID)
{
	__volatile__ ProcessAddressSpace* pPAS = &ProcessManager_processAddressSpace[iProcessID] ;
	__volatile__ ProcessStateInfo* pStat = pPAS->pProcessStateInfo ;

	if(pPAS->status == TERMINATED)
	{
		// Parent Responsible for freeing address space of terminated child process
		return ;
	}

	switch(pPAS->status)
	{
	case WAIT_SLEEP:
		{
			if(PIT_GetClockCount() >= pStat->uiSleepTime)
			{
				pStat->uiSleepTime = 0 ;
				pPAS->status = RUN ;
			}
			else
			{
				if(debug_point) 
				{
					printf("\n Sleep Time: %u", pStat->uiSleepTime) ;
					printf("\n PIT Tick Count: %u", PIT_GetClockCount()) ;
					printf("\n") ;
				}
				return ;
			}
		}
		break ;

	case WAIT_INT:
		{
			if(!ProcessManager_WakeupProcessOnInterrupt(iProcessID))
				return ;
		}
		break ;
	
	case WAIT_CHILD:
		{
			if(pStat->iWaitChildProcId < 0)
			{
				pStat->iWaitChildProcId = NO_PROCESS_ID ;
				pPAS->status = RUN ;
			}
			else
			{
				ProcessAddressSpace* p = &ProcessManager_processAddressSpace[pStat->iWaitChildProcId] ;

				if(p->bFree == true || p->iParentProcessID != iProcessID)
				{
					pStat->iWaitChildProcId = NO_PROCESS_ID ;
					pPAS->status = RUN ;
				}
				else if(p->status == TERMINATED && p->iParentProcessID == iProcessID)
				{
					ProcessManager_Release(pStat->iWaitChildProcId) ;
					pStat->iWaitChildProcId = NO_PROCESS_ID ;
					pPAS->status = RUN ;
				}
				else
					return ;
			}
		}
		break ;

	case WAIT_RESOURCE:
		{
			if(pStat->uiWaitResourceId == RESOURCE_NIL)
			{
				pPAS->status = RUN ;
			}
			else
			{
				if(ProcessManager_ResourceList[pStat->uiWaitResourceId] == false)
				{ 
					pStat->uiWaitResourceId = RESOURCE_NIL ;
					pPAS->status = RUN ;
				}
				else
					return ;
			}
		}
		break ;

	case WAIT_KERNEL_SERVICE:
		{
			ProcessAddressSpace* p = &ProcessManager_processAddressSpace[iProcessID] ;
			if(p->pProcessStateInfo->bKernelServiceComplete)
			{
				p->pProcessStateInfo->bKernelServiceComplete = false ;
				p->status = RUN ;
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

	ProcessManager_Load(iProcessID) ;

	PIT_SetContextSwitch(false) ;

	KERNEL_MODE = false ;
	/* Switch Insturction */
	__asm__ __volatile__("lcall $0x28, $0x00") ;
	KERNEL_MODE = true ;

	ProcessManager_Store(iProcessID) ;

	if(PIT_IsContextSwitch() == false || pPAS->status == TERMINATED)
		ProcessManager_Destroy(iProcessID) ;

	if(debug_point) TRACE_LINE ;
	//PIC::EnableInterrupt(TIMER_IRQ) ;
	PIT_SetTaskSwitch(true) ;
	if(debug_point) TRACE_LINE ;
}

void ProcessManager_StartScheduler()
{
	while(ProcessManager_uiCount > 0)
	{
		__volatile__ ProcessAddressSpace* pPAS = &ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID] ;

		if(pPAS->status == TERMINATED && pPAS->iParentProcessID == MOSMain_KernelProcessID())
		{
			int pid = pPAS->iNextProcessID ;
			ProcessManager_Release(ProcessManager_iCurrentProcessID) ;
			ProcessManager_iCurrentProcessID = pid ;
			continue ;
		}

		ProcessManager_DoContextSwitch(ProcessManager_iCurrentProcessID) ;

		if(ProcessManager_iCurrentProcessID != NO_PROCESS_ID && PIT_IsTaskSwitch())
		{
			ProcessManager_iCurrentProcessID = ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].iNextProcessID ;
		}
	}
}

void ProcessManager_EnableTaskSwitch()
{
	PIT_SetTaskSwitch(true) ;
}

void ProcessManager_DisableTaskSwitch()
{
	PIT_SetTaskSwitch(false) ;
}

void ProcessManager_Sleep(__volatile__ unsigned uiSleepTime) // in Mili Seconds
{
	if(ProcessManager_DoPoleWait())
	{
		KernelUtil::Wait(uiSleepTime) ;
		return ;
	}

	ProcessManager_DisableTaskSwitch() ;

	uiSleepTime = PIT_RoundSleepTime(uiSleepTime) ;
	ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].pProcessStateInfo->uiSleepTime = PIT_GetClockCount() + uiSleepTime ;
	ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].status = WAIT_SLEEP ;

	ProcessManager_Yield() ;
}

void ProcessManager_WaitOnInterrupt(const IRQ* pIRQ)
{
	if(ProcessManager_DoPoleWait())
	{
		KernelUtil::WaitOnInterrupt(pIRQ) ;
		return ;
	}

	ProcessManager_DisableTaskSwitch() ;
	
	ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].pProcessStateInfo->pIRQ = pIRQ ;
	ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].status = WAIT_INT ;

	ProcessManager_Yield() ;
}

void ProcessManager_WaitOnChild(int iChildProcessID)
{
	if(ProcessManager_GetCurProcId() < 0)
		return ;

	if(iChildProcessID < 0 || iChildProcessID >= MAX_NO_PROCESS)
		return ;

	ProcessManager_DisableTaskSwitch() ;
	
	ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].pProcessStateInfo->iWaitChildProcId = iChildProcessID ;
	ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].status = WAIT_CHILD ;

	ProcessManager_Yield() ;
}

void ProcessManager_WaitOnResource(unsigned uiResourceType)
{
	if(ProcessManager_GetCurProcId() < 0)
		return ;

	//ProcessManager_EnableTaskSwitch() ;
	ProcessManager_DisableTaskSwitch() ;
	
	ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].pProcessStateInfo->uiWaitResourceId = uiResourceType ;
	ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].status = WAIT_RESOURCE ;

	ProcessManager_Yield() ;
}

byte ProcessManager_IsChildAlive(int iChildProcessID)
{
	ProcessManager_DisableTaskSwitch() ;

	byte status = true ;

	if(iChildProcessID < 0 || iChildProcessID >= MAX_NO_PROCESS)
		status = false ;
	else
	{
		ProcessAddressSpace* p = &ProcessManager_processAddressSpace[iChildProcessID] ;

		if(p->bFree == true || p->iParentProcessID != ProcessManager_iCurrentProcessID)
			status = false ;
	}

	ProcessManager_EnableTaskSwitch() ;

	return status ;
}

byte ProcessManager_CreateKernelImage(const unsigned uiTaskAddress, int iParentProcessID, byte bIsFGProcess, unsigned uiParam1, unsigned uiParam2, 
		int* iProcessID, const char* szKernelProcName)
{
	byte bStatus ;

	int iNewProcessID ;
	RETURN_IF_NOT(bStatus, ProcessManager_FindFreePAS(iNewProcessID), ProcessManager_SUCCESS) ;

	if(iParentProcessID != NO_PROCESS_ID)
	{
		ProcessManager_processAddressSpace[iNewProcessID].iDriveID = ProcessManager_processAddressSpace[iParentProcessID].iDriveID ;

		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(ProcessManager_processAddressSpace[iParentProcessID].processPWD),
		MemUtil_GetDS(), (unsigned)&(ProcessManager_processAddressSpace[iNewProcessID].processPWD), 
		sizeof(FileSystem_PresentWorkingDirectory)) ;
	}
	else
	{
		int iDriveID = ROOT_DRIVE_ID ;
		ProcessManager_processAddressSpace[iNewProcessID].iDriveID = iDriveID ;

		if(iDriveID != CURRENT_DRIVE)
		{
			DriveInfo* pDriveInfo = DeviceDrive_GetByID(iDriveID, false) ;
			if(pDriveInfo == NULL)
				return ProcessManager_FAILURE ;

			if(pDriveInfo->drive.bMounted)
			{
				MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pDriveInfo->FSMountInfo.FSpwd), MemUtil_GetDS(), 
						(unsigned)&ProcessManager_processAddressSpace[iNewProcessID].processPWD, sizeof(FileSystem_PresentWorkingDirectory)) ;
			}
		}
	}

	unsigned uiStackAddress ;
	RETURN_X_IF_NOT(ProcessAllocator_AllocateAddressSpaceForKernel(&ProcessManager_processAddressSpace[iNewProcessID], 
		&uiStackAddress), ProcessAllocator_SUCCESS, ProcessManager_FAILURE) ;

	ProcessManager_processAddressSpace[iNewProcessID].bIsKernelProcess = true ;

	ProcessEnv_InitializeForKernelProcess() ;

//	DriveInfo* pDriveInfo = DeviceDrive_GetByID(ProcessManager_processAddressSpace[iNewProcessID].iDriveID) ;
//
//	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pDriveInfo->FSMountInfo.FSpwd), 
//	MemUtil_GetDS(), (unsigned)&ProcessManager_processAddressSpace[iNewProcessID].processPWD, sizeof(FileSystem_PresentWorkingDirectory)) ;

	unsigned uiStackTop = uiStackAddress - GLOBAL_DATA_SEGMENT_BASE + (PROCESS_KERNEL_STACK_PAGES * PAGE_SIZE) - 1 ;

	ProcessManager_BuildKernelTaskState(uiTaskAddress, iNewProcessID, uiStackTop, uiParam1, uiParam2) ;
	ProcessManager_BuildKernelTaskLDT(iNewProcessID) ;
	
	ProcessManager_processAddressSpace[iNewProcessID].iUserID = ROOT_USER_ID ;

	ProcessManager_processAddressSpace[iNewProcessID].iParentProcessID = iParentProcessID ;
	int iProcessGroupID ;

	if(iParentProcessID == NO_PROCESS_ID)
	{
		if(ProcessGroupManager_CreateProcessGroup(bIsFGProcess, &iProcessGroupID) != ProcessGroupManager_SUCCESS)
			return ProcessManager_FAILURE ;
	}
	else
	{
		iProcessGroupID = ProcessManager_processAddressSpace[iParentProcessID].iProcessGroupID ;
	}

	ProcessManager_processAddressSpace[iNewProcessID].iProcessGroupID = iProcessGroupID ;
	ProcessGroupManager_AddProcessToGroup(iNewProcessID) ;

	*iProcessID = iNewProcessID ;

	if(bIsFGProcess)
		ProcessGroupManager_PutOnFGProcessList(iNewProcessID) ;

	if(szKernelProcName == NULL)
		szKernelProcName = DEF_KERNEL_PROC_NAME ;

	ProcessManager_processAddressSpace[iNewProcessID].pname = (char*)DMM_AllocateForKernel(String_Length(szKernelProcName) + 1);
	String_Copy(ProcessManager_processAddressSpace[iNewProcessID].pname, szKernelProcName) ;

	ProcessStateInfo* pStateInfo = (ProcessStateInfo*)DMM_AllocateForKernel(sizeof(ProcessStateInfo)) ;
	
	pStateInfo->uiSleepTime = 0 ;
	pStateInfo->pIRQ = &PIC::NO_IRQ ;
	pStateInfo->iWaitChildProcId = NO_PROCESS_ID ;
	pStateInfo->uiWaitResourceId = RESOURCE_NIL ;

	ProcessManager_processAddressSpace[iNewProcessID].pProcessStateInfo = pStateInfo ;
	ProcessManager_processAddressSpace[iNewProcessID].status = RUN ;

	ProcessManager_AddToSchedulerList(iNewProcessID) ;

	return ProcessManager_SUCCESS ;
}

byte ProcessManager_Create(const char* szProcessName, int iParentProcessID, byte bIsFGProcess, int* iProcessID, int iUserID, int iNumberOfParameters, char** szArgumentList)
{
	//KC::MMemManager().DisplayNoOfFreePages();
	byte bStatus ;

	int iNewProcessID ;
	RETURN_IF_NOT(bStatus, ProcessManager_FindFreePAS(iNewProcessID), ProcessManager_SUCCESS) ;

	unsigned uiEntryAdddress ;
	unsigned uiProcessEntryStackSize ;
	unsigned uiPDEAddress ;
	
	if(ProcessLoader_Load(szProcessName, &ProcessManager_processAddressSpace[iNewProcessID], &uiPDEAddress,
		 &uiEntryAdddress, &uiProcessEntryStackSize, iNumberOfParameters, szArgumentList) != ProcessLoader_SUCCESS)
	{
		return ProcessManager_FAILURE ;
	}

	if(iParentProcessID != NO_PROCESS_ID)
	{
		ProcessManager_processAddressSpace[iNewProcessID].iDriveID = ProcessManager_processAddressSpace[iParentProcessID].iDriveID ;

		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(ProcessManager_processAddressSpace[iParentProcessID].processPWD),
		MemUtil_GetDS(), (unsigned)&(ProcessManager_processAddressSpace[iNewProcessID].processPWD), 
		sizeof(FileSystem_PresentWorkingDirectory)) ;
	}
	else
	{
		int iDriveID = ROOT_DRIVE_ID ;
		ProcessManager_processAddressSpace[iNewProcessID].iDriveID = iDriveID ;

		if(iDriveID != CURRENT_DRIVE)
		{
			DriveInfo* pDriveInfo = DeviceDrive_GetByID(iDriveID, false) ;
			if(pDriveInfo == NULL)
				return ProcessManager_FAILURE ;

			if(pDriveInfo->drive.bMounted)
			{
				MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pDriveInfo->FSMountInfo.FSpwd), MemUtil_GetDS(), 
						(unsigned)&ProcessManager_processAddressSpace[iNewProcessID].processPWD, sizeof(FileSystem_PresentWorkingDirectory)) ;
			}
		}
	}

	RETURN_X_IF_NOT(ProcessEnv_Initialize(uiPDEAddress, iParentProcessID), ProcessEnv_SUCCESS, ProcessManager_FAILURE) ;

	RETURN_X_IF_NOT(ProcFileManager_Initialize(uiPDEAddress, iParentProcessID), ProcessEnv_SUCCESS, ProcessManager_FAILURE) ;

	ProcessManager_processAddressSpace[iNewProcessID].bIsKernelProcess = false ;
	ProcessManager_processAddressSpace[iNewProcessID].uiNoOfPagesForDLLPTE = 0 ;

	ProcessManager_BuildLDT(&ProcessManager_processAddressSpace[iNewProcessID].processLDT) ;
	ProcessManager_BuildTaskState(&ProcessManager_processAddressSpace[iNewProcessID], uiPDEAddress, uiEntryAdddress, uiProcessEntryStackSize) ;

	if(iUserID == DERIVE_FROM_PARENT)
	{
		ProcessManager_processAddressSpace[iNewProcessID].iUserID = ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].iUserID ;
	}
	else
	{ 
		ProcessManager_processAddressSpace[iNewProcessID].iUserID = iUserID ;
	}

	*iProcessID = iNewProcessID ;

	ProcessManager_processAddressSpace[iNewProcessID].iParentProcessID = iParentProcessID ;
	int iProcessGroupID ;

	if(iParentProcessID == NO_PROCESS_ID)
	{
		if(ProcessGroupManager_CreateProcessGroup(bIsFGProcess, &iProcessGroupID) != ProcessGroupManager_SUCCESS)
		{
			return ProcessManager_FAILURE ;
		}
	}
	else
	{
		ProcessManager_processAddressSpace[iNewProcessID].iDriveID = ProcessManager_processAddressSpace[iParentProcessID].iDriveID ;
		iProcessGroupID = ProcessManager_processAddressSpace[iParentProcessID].iProcessGroupID ;
	}

	ProcessManager_processAddressSpace[iNewProcessID].iProcessGroupID = iProcessGroupID ;
	ProcessGroupManager_AddProcessToGroup(iNewProcessID) ;

	*iProcessID = iNewProcessID ;

	if(bIsFGProcess)
		ProcessGroupManager_PutOnFGProcessList(iNewProcessID) ;

	DisplayManager::SetupPageTableForDisplayBuffer(iProcessGroupID, uiPDEAddress) ;

	ProcessManager_processAddressSpace[iNewProcessID].pname = (char*)DMM_AllocateForKernel(String_Length(szProcessName) + 1) ;
	String_Copy(ProcessManager_processAddressSpace[iNewProcessID].pname, szProcessName) ;

	// Init Process State Info
	ProcessStateInfo* pStateInfo = (ProcessStateInfo*)DMM_AllocateForKernel(sizeof(ProcessStateInfo)) ;
	
	pStateInfo->uiSleepTime = 0 ;
	pStateInfo->pIRQ = &PIC::NO_IRQ ;
	pStateInfo->iWaitChildProcId = NO_PROCESS_ID ;
	pStateInfo->uiWaitResourceId = RESOURCE_NIL ;

	ProcessManager_processAddressSpace[iNewProcessID].pProcessStateInfo = pStateInfo ;
	ProcessManager_processAddressSpace[iNewProcessID].status = RUN ;

	ProcessManager_AddToSchedulerList(iNewProcessID) ;

	//KC::MMemManager().DisplayNoOfFreePages() ;
	return ProcessManager_SUCCESS ;
}

byte ProcessManager_GetFromInterruptQueue(const IRQ* pIRQ)
{
	unsigned val ;

	PIC::DisableInterrupt(pIRQ->GetIRQNo()) ;
	
	ProcessManager_ConsumeInterrupt(pIRQ) ;

	if(DSUtil_ReadFromQueue(&(ProcessManager_InterruptQueue[pIRQ->GetIRQNo()]), &val) == DSUtil_ERR_QUEUE_EMPTY)
	{
		PIC::EnableInterrupt(pIRQ->GetIRQNo()) ;
		return ProcessManager_ERR_INT_QUEUE_EMPTY ;
	}

	PIC::EnableInterrupt(pIRQ->GetIRQNo()) ;
	return ProcessManager_SUCCESS ;
}

byte ProcessManager_QueueInterrupt(const IRQ* pIRQ)
{
	if(DSUtil_WriteToQueue(&(ProcessManager_InterruptQueue[pIRQ->GetIRQNo()]), 1) == DSUtil_ERR_QUEUE_FULL)
		return ProcessManager_ERR_INT_QUEUE_FULL ;
	
	return ProcessManager_SUCCESS ;
}

void ProcessManager_SignalInterruptOccured(const IRQ* pIRQ)
{
	ProcessManager_SetInterruptOccured(pIRQ) ;

	if(ProcessManager_QueueInterrupt(pIRQ) == ProcessManager_ERR_INT_QUEUE_FULL)
		return ;
}

static byte ProcessManager_GetProcList_ASync(PS** pProcList, unsigned* uiListSize)
{
	*uiListSize = ProcessManager_uiCount ;
	PS* pPS ;
	ProcessAddressSpace* pAddrSpc = &ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID] ;

	if(pAddrSpc->bIsKernelProcess == true)
	{
		*pProcList = (PS*)DMM_AllocateForKernel(sizeof(PS) * ProcessManager_uiCount) ;

		if(*pProcList == NULL)
			return ProcessManager_FAILURE ;

		pPS = *pProcList ;
	}
	else
	{
		*pProcList = (PS*)DMM_Allocate(pAddrSpc, sizeof(PS) * ProcessManager_uiCount) ;

		if(*pProcList == NULL)
			return ProcessManager_FAILURE ;

		pPS = (PS*)(((unsigned)*pProcList + PROCESS_BASE) - GLOBAL_DATA_SEGMENT_BASE) ;
	}

	DSUtil_SLLNode* pCur = ProcessManager_ProcessList.pFirst ;
	int i ;
	
	for(i = 0; pCur != NULL; pCur = pCur->pNext, i++)
	{
		ProcessAddressSpace* p = &ProcessManager_processAddressSpace[pCur->val] ;
		
		pPS[i].pid = pCur->val ;
		pPS[i].status = p->status ;
		pPS[i].iParentProcessID = p->iParentProcessID ;
		pPS[i].iProcessGroupID = p->iProcessGroupID ;
		pPS[i].iUserID = p->iUserID ;

		char* pname ;
		if(pAddrSpc->bIsKernelProcess == true)
		{
			pPS[i].pname = (char*)DMM_AllocateForKernel(String_Length(p->pname) + 1) ;
			if(pPS[i].pname == NULL)
				return ProcessManager_FAILURE ;
			pname = pPS[i].pname ;
		}
		else
		{
			pPS[i].pname = (char*)DMM_Allocate(pAddrSpc, String_Length(p->pname) + 1) ;
			if(pPS[i].pname == NULL)
				return ProcessManager_FAILURE ;
			pname = (char*)((unsigned)pPS[i].pname + PROCESS_BASE - GLOBAL_DATA_SEGMENT_BASE) ;
		}
		
		String_Copy(pname, p->pname) ;
	}

	return ProcessManager_SUCCESS ;
}

byte ProcessManager_GetProcList(PS** pProcList, unsigned* uiListSize)
{
	byte bStatus ;
	ProcessManager_DisableTaskSwitch() ;
	bStatus = ProcessManager_GetProcList_ASync(pProcList, uiListSize) ;
	ProcessManager_EnableTaskSwitch() ;
	return bStatus ;
}

void ProcessManager_FreeProcListMem(PS* pProcList, unsigned uiListSize)
{
	ProcessAddressSpace* pAddrSpc = &ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID] ;

	unsigned i ;
	for(i = 0; i < uiListSize; i++)
	{
		if(pAddrSpc->bIsKernelProcess == true)
		{
			DMM_DeAllocateForKernel((unsigned)pProcList[i].pname) ;
		}
		else
		{
			DMM_DeAllocate(pAddrSpc, (unsigned)pProcList[i].pname) ;
		}
	}
	
	if(ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].bIsKernelProcess == true)
	{
		DMM_DeAllocateForKernel((unsigned)pProcList) ;
	}
	else
	{
		DMM_DeAllocate(pAddrSpc, (unsigned)pProcList) ;
	}
}

void ProcessManager_SetDMMFlag(int iProcessID, bool flag)
{
	ProcessManager_processAddressSpace[iProcessID].uiDMMFlag = flag ;
}

bool ProcessManager_IsDMMOn(int iProcessID)
{
	return ProcessManager_processAddressSpace[iProcessID].uiDMMFlag ;
}

bool ProcessManager_IsKernelProcess(int iProcessID)
{
	return ProcessManager_processAddressSpace[ iProcessID ].bIsKernelProcess ;
}

void ProcessManager_Exit()
{
	PIT_SetContextSwitch(false) ;
	ProcessManager_EXIT() ;
}

bool ProcessManager_ConsumeInterrupt(const IRQ* pIRQ)
{
	return Atomic::Swap((int&)ProcessManager_InterruptOccured[ pIRQ->GetIRQNo() ], false) ;
}

void ProcessManager_SetInterruptOccured(const IRQ* pIRQ)
{
	Atomic::Swap((int&)ProcessManager_InterruptOccured[ pIRQ->GetIRQNo() ], true) ;
}

bool ProcessManager_IsResourceBusy(__volatile__ unsigned uiType)
{
	return ProcessManager_ResourceList[ uiType ] ;
}

void ProcessManager_SetResourceBusy(unsigned uiType, bool bVal)
{
	ProcessManager_ResourceList[ uiType ] = bVal ;
}

int ProcessManager_GetCurProcId()
{
	return KERNEL_MODE ? -1 : ProcessManager_iCurrentProcessID ;
}

void ProcessManager_Kill(int iProcessID)
{
	ProcessAddressSpace* pPAS = &ProcessManager_processAddressSpace[iProcessID] ;
	Atomic::Swap((__volatile__ int&)(pPAS->status), static_cast<int>(TERMINATED)) ;
}

void ProcessManager_WakeUpFromKSWait(int iProcessID)
{
	ProcessAddressSpace* pPAS = &ProcessManager_processAddressSpace[iProcessID] ;
	pPAS->pProcessStateInfo->bKernelServiceComplete = true ;
}

void ProcessManager_WaitOnKernelService()
{
	if(ProcessManager_GetCurProcId() < 0)
		return ; 

	ProcessManager_DisableTaskSwitch() ;

	ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].pProcessStateInfo->bKernelServiceComplete = false ;
	ProcessManager_processAddressSpace[ProcessManager_iCurrentProcessID].status = WAIT_KERNEL_SERVICE ;

	ProcessManager_Yield() ;
}

bool ProcessManager_CopyDriveInfo(int iProcessID, int& iOldDriveId, FileSystem_PresentWorkingDirectory& mOldPWD)
{
	if(ProcessManager_GetCurProcId() < 0)
		return false ;

	ProcessAddressSpace* pSrcPAS = &ProcessManager_processAddressSpace[ iProcessID ] ;
	ProcessAddressSpace* pDestPAS = &ProcessManager_processAddressSpace[ ProcessManager_iCurrentProcessID ] ;

	iOldDriveId = pDestPAS->iDriveID ;
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pDestPAS->processPWD), MemUtil_GetDS(), (unsigned)&(mOldPWD), sizeof(FileSystem_PresentWorkingDirectory)) ;

	pDestPAS->iDriveID = pSrcPAS->iDriveID ;
	MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)&(pSrcPAS->processPWD), MemUtil_GetDS(), (unsigned)&(pDestPAS->processPWD), 
		sizeof(FileSystem_PresentWorkingDirectory)) ;

	return true ;
}

