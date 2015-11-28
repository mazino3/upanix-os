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
# include <ProcessGroupManager.h>
# include <ProcessManager.h>
# include <MemManager.h>
# include <Display.h>
# include <DMM.h>
# include <Atomic.h>

int ProcessGroupManager_iFGProcessGroup ;
ProcessGroup* ProcessGroupManager_AddressSpace ;

/***** static ****/
static Mutex& ProcessGroupManager_PGASMutex()
{
	static Mutex m ;
	return m ;
}

static int ProcessGroupManager_GetFreePGAS()
{
	ProcessGroupManager_PGASMutex().Lock() ;
	unsigned i ;
	for(i = 0; i < MAX_NO_PROCESS_GROUP; i++)
	{
		if(ProcessGroupManager_AddressSpace[i].bFree == true)
		{
			ProcessGroupManager_AddressSpace[i].bFree = false ;
			ProcessGroupManager_PGASMutex().UnLock() ;
			return i ;
		}
	}

	ProcessGroupManager_PGASMutex().UnLock() ;
	return -1 ;
}

static void ProcessGroupManager_FreePGAS(int iProcessGroupID)
{
	ProcessGroupManager_PGASMutex().Lock() ;
	ProcessGroupManager_AddressSpace[iProcessGroupID].bFree = true ;
	ProcessGroupManager_PGASMutex().UnLock() ;
}

/*********************/

byte ProcessGroupManager_Initialize()
{
	if(((sizeof(ProcessGroup) * MAX_NO_PROCESS_GROUP)) > (MEM_PGAS_END - MEM_PGAS_START))
	{
		KC::MDisplay().Message("\n Process Group Address Space InSufficient\n", '\r') ;
		return ProcessGroupManager_FAILURE ;
	}

	ProcessGroupManager_iFGProcessGroup = NO_PROCESS_GROUP_ID ;

	ProcessGroupManager_AddressSpace = (ProcessGroup*)MEM_PGAS_START ;

	unsigned i ;
	for(i = 0; i < MAX_NO_PROCESS_GROUP; i++)
		ProcessGroupManager_AddressSpace[i].bFree = true ;

	return ProcessGroupManager_SUCCESS ;
}

int ProcessGroupManager_GetProcessCount(int iProcessGroupID)
{
	return ProcessGroupManager_AddressSpace[iProcessGroupID].iProcessCount ;
}

byte ProcessGroupManager_CreateProcessGroup(byte bIsFGProcessGroup, int* iProcessGroupID)
{
	int iPgid = ProcessGroupManager_GetFreePGAS() ;

	if(iPgid == -1)
		return ProcessGroupManager_ERR_NO_FREE_PGAS ;

	ProcessGroupManager_AddressSpace[iPgid].iProcessCount = 0 ;
	ProcessGroupManager_AddressSpace[iPgid].fgProcessListHead.pNext = NULL ;
	ProcessGroupManager_AddressSpace[iPgid].fgProcessListHead.iProcessID = NO_PROCESS_ID ;
	
	if(bIsFGProcessGroup)
		ProcessGroupManager_iFGProcessGroup = iPgid ;

	unsigned uiFreePageNo ;
	RETURN_X_IF_NOT(MemManager::Instance().AllocatePhysicalPage(&uiFreePageNo), Success, ProcessGroupManager_FAILURE) ;

	DisplayManager::InitializeDisplayBuffer(ProcessGroupManager_AddressSpace[iPgid].videoBuffer, uiFreePageNo * PAGE_SIZE) ;

	*iProcessGroupID = iPgid ;

	return ProcessGroupManager_SUCCESS ;
}

byte ProcessGroupManager_DestroyProcessGroup(int iProcessGroupID)
{
	ProcessGroupManager_FreePGAS(iProcessGroupID) ;
	ProcessGroup* pGroup = &ProcessGroupManager_AddressSpace[iProcessGroupID] ;

	MemManager::Instance().DeAllocatePhysicalPage(((unsigned)pGroup->videoBuffer.GetDisplayMemAddr()) / PAGE_SIZE) ;

	if(pGroup->fgProcessListHead.pNext != NULL)
		DMM_DeAllocateForKernel((unsigned)pGroup->fgProcessListHead.pNext) ;

	return ProcessGroupManager_SUCCESS ;
}

void ProcessGroupManager_PutOnFGProcessList(int iProcessID)
{
	ProcessGroupManager_PGASMutex().Lock() ;

	ProcessAddressSpace* pAddrSpace = &ProcessManager_processAddressSpace[iProcessID] ;
	ProcessGroup* pGroup = &ProcessGroupManager_AddressSpace[pAddrSpace->iProcessGroupID] ;

	FGProcessList* pFGProcessListEntry = (FGProcessList*)DMM_AllocateForKernel(sizeof(FGProcessList)) ;
	pFGProcessListEntry->iProcessID = iProcessID ;
	pFGProcessListEntry->pNext = pGroup->fgProcessListHead.pNext ;
	pGroup->fgProcessListHead.pNext = pFGProcessListEntry ;

	ProcessGroupManager_PGASMutex().UnLock() ;
}

void ProcessGroupManager_RemoveFromFGProcessList(int iProcessID)
{
	ProcessGroupManager_PGASMutex().Lock() ;

	ProcessAddressSpace* pAddrSpace = &ProcessManager_processAddressSpace[iProcessID] ;
	ProcessGroup* pGroup = &ProcessGroupManager_AddressSpace[pAddrSpace->iProcessGroupID] ;
	
	FGProcessList* pPrev = &pGroup->fgProcessListHead, *pCur ;
	for(pCur = pGroup->fgProcessListHead.pNext; pCur != NULL; pCur = pCur->pNext)
	{
		if(pCur->iProcessID == iProcessID)
		{
			pPrev->pNext = pCur->pNext ;
			DMM_DeAllocateForKernel((unsigned)pCur) ;
			break ;
		}

		pPrev = pCur ;
	}

	ProcessGroupManager_PGASMutex().UnLock() ;
}

void ProcessGroupManager_AddProcessToGroup(int iProcessID)
{
	ProcessGroupManager_PGASMutex().Lock() ;

	ProcessAddressSpace* pAddrSpace = &ProcessManager_processAddressSpace[iProcessID] ;
	ProcessGroup* pGroup = &ProcessGroupManager_AddressSpace[pAddrSpace->iProcessGroupID] ;
	pGroup->iProcessCount++ ;

	ProcessGroupManager_PGASMutex().UnLock() ;
}

void ProcessGroupManager_RemoveProcessFromGroup(int iProcessID)
{
	ProcessGroupManager_PGASMutex().Lock() ;

	ProcessAddressSpace* pAddrSpace = &ProcessManager_processAddressSpace[iProcessID] ;
	ProcessGroup* pGroup = &ProcessGroupManager_AddressSpace[pAddrSpace->iProcessGroupID] ;
	pGroup->iProcessCount-- ;
	
	ProcessGroupManager_PGASMutex().UnLock() ;
}

void ProcessGroupManager_SwitchFGProcessGroup(int iProcessID)
{
	ProcessGroupManager_PGASMutex().Lock() ;

	ProcessGroupManager_iFGProcessGroup = ProcessManager_processAddressSpace[iProcessID].iProcessGroupID ;

	ProcessGroupManager_PGASMutex().UnLock() ;
}

bool ProcessGroupManager_IsFGProcessGroup(int iProcessGroupID)
{
	return (iProcessGroupID == ProcessGroupManager_iFGProcessGroup) ;
}

bool ProcessGroupManager_IsFGProcess(int iProcessGroupID, int iProcessID)
{
	ProcessGroup* pGroup = &ProcessGroupManager_AddressSpace[iProcessGroupID] ;

	if(pGroup->fgProcessListHead.pNext != NULL)
	{
		return (pGroup->fgProcessListHead.pNext->iProcessID == iProcessID) ;
	}

	return false ;
}

const ProcessGroup* ProcessGroupManager_GetFGProcessGroup()
{
	return &ProcessGroupManager_AddressSpace[ProcessGroupManager_iFGProcessGroup] ;
}
