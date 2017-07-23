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
#include <MemManager.h>
#include <ProcessAllocator.h>
#include <DMM.h>

byte
ProcessAllocator_AllocateAddressSpaceForKernel(Process* processAddressSpace, unsigned* uiStackAddress)
{
	unsigned i, uiFreePageNo ;
	
	int iStackBlockID = MemManager::Instance().GetFreeKernelProcessStackBlockID() ;
	
	if(iStackBlockID < 0)
		return ProcessAllocator_FAILURE ;

	*uiStackAddress = KERNEL_PROCESS_PDE_ID * PAGE_TABLE_ENTRIES * PAGE_SIZE + iStackBlockID * PROCESS_KERNEL_STACK_PAGES * PAGE_SIZE;

	processAddressSpace->iKernelStackBlockID = iStackBlockID ;

	for(i = 0; i < PROCESS_KERNEL_STACK_PAGES; i++)
	{
    uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
		MemManager::Instance().GetKernelProcessStackPTEBase()[iStackBlockID * PROCESS_KERNEL_STACK_PAGES + i] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x3 ;
	}
  Mem_FlushTLB();

	return ProcessAllocator_SUCCESS ;
}

void
ProcessAllocator_DeAllocateAddressSpaceForKernel(Process* processAddressSpace)
{
	unsigned i ;
	const int iStackBlockID = processAddressSpace->iKernelStackBlockID ;

	for(i = 0; i < PROCESS_KERNEL_STACK_PAGES; i++)
	{
		MemManager::Instance().DeAllocatePhysicalPage(
		(MemManager::Instance().GetKernelProcessStackPTEBase()[iStackBlockID * PROCESS_KERNEL_STACK_PAGES + i] & 0xFFFFF000) / PAGE_SIZE) ;
	}
  Mem_FlushTLB();

	MemManager::Instance().FreeKernelProcessStackBlock(iStackBlockID) ;
}

byte ProcessAllocator_AllocatePagesForDLL(unsigned uiNoOfPagesForDLL, ProcessSharedObjectList* pProcessSharedObjectList)
{
	unsigned uiFreePageNo ;
	unsigned i ;

	pProcessSharedObjectList->uiAllocatedPageNumbers = (unsigned*)DMM_AllocateForKernel(sizeof(unsigned) * uiNoOfPagesForDLL) ;
	for(i = 0; i < uiNoOfPagesForDLL; i++)
	{
    uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
		pProcessSharedObjectList->uiAllocatedPageNumbers[i] = uiFreePageNo ;	
	}

	pProcessSharedObjectList->uiNoOfPages = uiNoOfPagesForDLL ;
	return ProcessAllocator_SUCCESS ;
}

