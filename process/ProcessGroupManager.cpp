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
# include <Exerr.h>

ProcessGroupManager::ProcessGroupManager() : 
  _iFGProcessGroup(NO_PROCESS_GROUP_ID), 
  _groups((ProcessGroup*)MEM_PGAS_START)
{
	if(((sizeof(ProcessGroup) * MAX_NO_PROCESS_GROUP)) > (MEM_PGAS_END - MEM_PGAS_START))
    throw Exerr(XLOC, "Process Group Address Space InSufficient");

	for(int i = 0; i < MAX_NO_PROCESS_GROUP; i++)
		_groups[i].bFree = true ;
}

int ProcessGroupManager::GetFreePGAS()
{
  MutexGuard g(_mutex);
	for(unsigned i = 0; i < MAX_NO_PROCESS_GROUP; i++)
	{
		if(_groups[i].bFree == true)
		{
			_groups[i].bFree = false ;
			return i;
		}
	}
  throw Exerr(XLOC, "out of free process group memory");
}

void ProcessGroupManager::FreePGAS(int iProcessGroupID)
{
  MutexGuard g(_mutex);
	_groups[iProcessGroupID].bFree = true ;
}

int ProcessGroupManager::GetProcessCount(int iProcessGroupID)
{
	return _groups[iProcessGroupID].iProcessCount ;
}

int ProcessGroupManager::CreateProcessGroup(bool isFGProcessGroup)
{
	int iPgid = GetFreePGAS() ;

	_groups[iPgid].iProcessCount = 0 ;
	_groups[iPgid].fgProcessListHead.pNext = NULL ;
	_groups[iPgid].fgProcessListHead.iProcessID = NO_PROCESS_ID ;
	
	if(isFGProcessGroup)
		_iFGProcessGroup = iPgid ;

	unsigned uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();

	DisplayManager::InitializeDisplayBuffer(_groups[iPgid].videoBuffer, uiFreePageNo * PAGE_SIZE) ;
  return iPgid;
}

void ProcessGroupManager::DestroyProcessGroup(int iProcessGroupID)
{
	FreePGAS(iProcessGroupID) ;
	ProcessGroup& pGroup = _groups[iProcessGroupID] ;

	MemManager::Instance().DeAllocatePhysicalPage(((unsigned)pGroup.videoBuffer.GetDisplayMemAddr()) / PAGE_SIZE) ;

	if(pGroup.fgProcessListHead.pNext != NULL)
		DMM_DeAllocateForKernel((unsigned)pGroup.fgProcessListHead.pNext) ;
}

void ProcessGroupManager::PutOnFGProcessList(int iProcessID)
{
  MutexGuard g(_mutex);

	ProcessAddressSpace& pAddrSpace = ProcessManager::Instance().GetAddressSpace(iProcessID);
	ProcessGroup& pGroup = _groups[pAddrSpace.iProcessGroupID];

	FGProcessList* pFGProcessListEntry = (FGProcessList*)DMM_AllocateForKernel(sizeof(FGProcessList)) ;
	pFGProcessListEntry->iProcessID = iProcessID ;
	pFGProcessListEntry->pNext = pGroup.fgProcessListHead.pNext ;
	pGroup.fgProcessListHead.pNext = pFGProcessListEntry ;
}

void ProcessGroupManager::RemoveFromFGProcessList(int iProcessID)
{
  MutexGuard g(_mutex);

	ProcessAddressSpace& pAddrSpace = ProcessManager::Instance().GetAddressSpace(iProcessID);
	ProcessGroup& pGroup = _groups[pAddrSpace.iProcessGroupID];
	
	FGProcessList* pPrev = &pGroup.fgProcessListHead, *pCur ;
	for(pCur = pGroup.fgProcessListHead.pNext; pCur != NULL; pCur = pCur->pNext)
	{
		if(pCur->iProcessID == iProcessID)
		{
			pPrev->pNext = pCur->pNext ;
			DMM_DeAllocateForKernel((unsigned)pCur) ;
			break ;
		}
		pPrev = pCur ;
	}
}

void ProcessGroupManager::AddProcessToGroup(int iProcessID)
{
  MutexGuard g(_mutex);
	ProcessAddressSpace& pAddrSpace = ProcessManager::Instance().GetAddressSpace(iProcessID);
	ProcessGroup& pGroup = _groups[pAddrSpace.iProcessGroupID] ;
	pGroup.iProcessCount++ ;
}

void ProcessGroupManager::RemoveProcessFromGroup(int iProcessID)
{
  MutexGuard g(_mutex);

	ProcessAddressSpace& pAddrSpace = ProcessManager::Instance().GetAddressSpace(iProcessID);
	ProcessGroup& pGroup = _groups[pAddrSpace.iProcessGroupID];
	pGroup.iProcessCount-- ;
}

void ProcessGroupManager::SwitchFGProcessGroup(int iProcessID)
{
  MutexGuard g(_mutex);
	_iFGProcessGroup = ProcessManager::Instance().GetAddressSpace(iProcessID).iProcessGroupID ;
}

bool ProcessGroupManager::IsFGProcessGroup(int iProcessGroupID)
{
	return iProcessGroupID == _iFGProcessGroup;
}

bool ProcessGroupManager::IsFGProcess(int iProcessGroupID, int iProcessID)
{
	ProcessGroup& pGroup = _groups[iProcessGroupID] ;
	if(pGroup.fgProcessListHead.pNext != NULL)
		return pGroup.fgProcessListHead.pNext->iProcessID == iProcessID;
	return false ;
}
