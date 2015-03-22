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
#include <MemManager.h>
#include <ProcessAllocator.h>
#include <DMM.h>

static byte ProcessAllocator_AllocatePDE(unsigned* uiPDEAddress) ;
static byte ProcessAllocator_AllocatePTE(unsigned* uiPDEAddress, unsigned* uiStartPDEForDLL, unsigned uiNoOfPagesForPTE, unsigned uiProcessBase) ;
static void ProcessAllocator_InitializeProcessSpaceForOS(unsigned* uiPDEAddress) ;
static byte ProcessAllocator_InitializeProcessSpaceForProcess(unsigned* uiPDEAddress, unsigned uiNoOfPagesForProcess, unsigned uiProcessBase) ;

static void ProcessAllocator_DeAllocateProcessSpace(unsigned uiNoOfPagesForProcess, unsigned uiPDEAddress, 
		unsigned uiProcessBase) ;
static void ProcessAllocator_DeAllocatePTE(unsigned uiNoOfPagesForPTE, unsigned uiPDEAddress, unsigned uiProcessBase) ;

byte ProcessAllocator_AllocateAddressSpace(const unsigned uiNoOfPagesForProcess, const unsigned uiNoOfPagesForPTE, 
		unsigned* uiPDEAddress, unsigned* uiStartPDEForDLL, unsigned uiProcessBase)
{
	RETURN_X_IF_NOT(ProcessAllocator_AllocatePDE(uiPDEAddress), ProcessAllocator_SUCCESS, ProcessAllocator_FAILURE) ;

	RETURN_X_IF_NOT(ProcessAllocator_AllocatePTE(uiPDEAddress, uiStartPDEForDLL, uiNoOfPagesForPTE, uiProcessBase), ProcessAllocator_SUCCESS, ProcessAllocator_FAILURE) ;

	ProcessAllocator_InitializeProcessSpaceForOS(uiPDEAddress) ;
	
	RETURN_X_IF_NOT(ProcessAllocator_InitializeProcessSpaceForProcess(uiPDEAddress, uiNoOfPagesForProcess, uiProcessBase), 
				ProcessAllocator_SUCCESS, ProcessAllocator_FAILURE) ;

	return ProcessAllocator_SUCCESS ;
}

static byte ProcessAllocator_AllocatePDE(unsigned* uiPDEAddress)
{
	unsigned uiFreePageNo, i ;

	RETURN_X_IF_NOT(KC::MMemManager().AllocatePhysicalPage(&uiFreePageNo), MEM_SUCCESS, ProcessAllocator_FAILURE) ;
	*uiPDEAddress = uiFreePageNo * PAGE_SIZE ;

	for(i = 0; i < PAGE_TABLE_ENTRIES; i++)
		((unsigned*)(*uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i] = 0x2 ;

	return ProcessAllocator_SUCCESS ;
}

static byte ProcessAllocator_AllocatePTE(unsigned* uiPDEAddress, unsigned* uiStartPDEForDLL, unsigned uiNoOfPagesForPTE, unsigned uiProcessBase)
{
	unsigned uiFreePageNo, i, j ;
	unsigned uiPDEIndex = 0 ;
	unsigned uiProcessPDEBase = uiProcessBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;

	for(i = 0; i < uiNoOfPagesForPTE; i++)
	{
		RETURN_X_IF_NOT(KC::MMemManager().AllocatePhysicalPage(&uiFreePageNo), MEM_SUCCESS, ProcessAllocator_FAILURE) ;

		for(j = 0; j < PAGE_TABLE_ENTRIES; j++)
			((unsigned*)((uiFreePageNo * PAGE_SIZE) - GLOBAL_DATA_SEGMENT_BASE))[j] = 0x2 ;

		if(i < PROCESS_SPACE_FOR_OS)
			// Full Permission is given at the PDE level and access control is enforced at PTE level i.e, individual page level
			((unsigned*)(*uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;
		else
		{
			uiPDEIndex = i + uiProcessPDEBase ;
			((unsigned*)(*uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;
		}
	}
	
	*uiStartPDEForDLL = ++uiPDEIndex ;
	
	return ProcessAllocator_SUCCESS ;
}

static void ProcessAllocator_InitializeProcessSpaceForOS(unsigned* uiPDEAddress)
{
	unsigned i, j ;
	unsigned uiPTEAddress ;
	unsigned uiPageCount = 0 ;
	
	for(i = 0; i < PROCESS_SPACE_FOR_OS; i++)
	{
		uiPTEAddress = (((unsigned*)(*uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i]) & 0xFFFFF000 ;
		for(j = 0; j < PAGE_TABLE_ENTRIES; j++)
		{
			((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[j] = ((uiPageCount * PAGE_SIZE) & 0xFFFFF000) | 0x3 ;
			uiPageCount++ ;
		}
	}

//	unsigned uiKernelHeapPageCount ;
//	unsigned uiPageAddress = MemManager_uiKernelHeapStartAddress ;	
//	for(i = PROCESS_SPACE_FOR_OS, uiKernelHeapPageCount = 0; 
//				i < (PROCESS_SPACE_FOR_OS); i++)
//	{
//		uiPTEAddress = (((unsigned*)(*uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i]) & 0xFFFFF000 ;
//		for(j = 0; j < PAGE_TABLE_ENTRIES && uiKernelHeapPageCount < MemManager_uiNoOfPagesForKernelHeap; 
//						j++, uiKernelHeapPageCount++)
//		{
//			((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[j] = (uiPageAddress & 0xFFFFF000) | 0x3 ;
//			uiPageAddress += PAGE_SIZE ;
//		}
//	}
}

static byte ProcessAllocator_InitializeProcessSpaceForProcess(unsigned* uiPDEAddress, unsigned uiNoOfPagesForProcess, unsigned uiProcessBase)
{
	unsigned uiFreePageNo, i ;
	unsigned uiPDEIndex, uiPTEIndex, uiPTEAddress ;
	unsigned uiProcessPDEBase = uiProcessBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;
	unsigned uiProcessPageBase = ( uiProcessBase / PAGE_SIZE ) % PAGE_TABLE_ENTRIES ;

	for(i = 0; i < uiNoOfPagesForProcess; i++)
	{
		RETURN_X_IF_NOT(KC::MMemManager().AllocatePhysicalPage(&uiFreePageNo), MEM_SUCCESS, ProcessAllocator_FAILURE) ;
		
		uiPDEIndex = (uiProcessPageBase + i) / PAGE_TABLE_ENTRIES + uiProcessPDEBase + PROCESS_SPACE_FOR_OS ;
		uiPTEIndex = (uiProcessPageBase + i) % PAGE_TABLE_ENTRIES ;
//Kernel Heap PTE and PROCESS_SPACE_FOR_OS need not be part of uiPTEIndex calculation as they shall be aligned at PAGE BOUNDARY

		uiPTEAddress = (((unsigned*)(*uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;

		if(i >= uiNoOfPagesForProcess - PROCESS_CG_STACK_PAGES)
		{
			((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x3 ;
		}
		else
		{
			((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;
		}
	}

	return ProcessAllocator_SUCCESS ;
}

void ProcessAllocator_DeAllocateProcessSpace(unsigned uiNoOfPagesForProcess, unsigned uiPDEAddress, unsigned uiProcessBase)
{
	unsigned i, uiPTEAddress ;
	unsigned uiPDEIndex, uiPTEIndex ;
	unsigned uiProcessPDEBase = uiProcessBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;
	unsigned uiProcessPageBase = ( uiProcessBase / PAGE_SIZE ) % PAGE_TABLE_ENTRIES ;

	for(i = 0; i < uiNoOfPagesForProcess; i++)
	{
		uiPDEIndex = (uiProcessPageBase + i) / PAGE_TABLE_ENTRIES + uiProcessPDEBase + PROCESS_SPACE_FOR_OS ;
		uiPTEIndex = (uiProcessPageBase + i) % PAGE_TABLE_ENTRIES ;
//Kernel Heap PTE and PROCESS_SPACE_FOR_OS need not be part of uiPTEIndex calculation as they shall
//be aligned at PAGE BOUNDARY

		uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;

		KC::MMemManager().DeAllocatePhysicalPage(
				(((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] & 0xFFFFF000) / PAGE_SIZE) ;
	}
}

void
ProcessAllocator_DeAllocatePTE(unsigned uiNoOfPagesForPTE, unsigned uiPDEAddress, unsigned uiProcessBase)
{
	unsigned i, uiPTEAddress ;
	unsigned uiProcessPDEBase = uiProcessBase / PAGE_SIZE / PAGE_TABLE_ENTRIES ;

	for(i = 0; i < uiNoOfPagesForPTE; i++)
	{
		if(i < PROCESS_SPACE_FOR_OS)
		{
			uiPTEAddress = ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i] & 0xFFFFF000 ;
		}
		else
		{
			uiPTEAddress = ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i + uiProcessPDEBase] & 0xFFFFF000 ;
		}
		KC::MMemManager().DeAllocatePhysicalPage(uiPTEAddress / PAGE_SIZE) ;
	}
}

void
ProcessAllocator_DeAllocateAddressSpace(ProcessAddressSpace* processAddressSpace)
{
	ProcessAllocator_DeAllocateProcessSpace(processAddressSpace->uiNoOfPagesForProcess, 
					processAddressSpace->taskState.CR3_PDBR,
					processAddressSpace->uiProcessBase) ;

	ProcessAllocator_DeAllocatePTE(processAddressSpace->uiNoOfPagesForPTE, 
					processAddressSpace->taskState.CR3_PDBR,
					processAddressSpace->uiProcessBase) ;

	KC::MMemManager().DeAllocatePhysicalPage(processAddressSpace->taskState.CR3_PDBR / PAGE_SIZE) ;
}

byte
ProcessAllocator_AllocateAddressSpaceForKernel(ProcessAddressSpace* processAddressSpace, unsigned* uiStackAddress)
{
	unsigned i, uiFreePageNo ;
	
	int iStackBlockID = KC::MMemManager().GetFreeKernelProcessStackBlockID() ;
	
	if(iStackBlockID < 0)
		return ProcessAllocator_FAILURE ;

	*uiStackAddress = KERNEL_PROCESS_PDE_ID * PAGE_TABLE_ENTRIES * PAGE_SIZE + iStackBlockID * PROCESS_KERNEL_STACK_PAGES * PAGE_SIZE ;

	processAddressSpace->iKernelStackBlockID = iStackBlockID ;

	for(i = 0; i < PROCESS_KERNEL_STACK_PAGES; i++)
	{
		RETURN_X_IF_NOT(KC::MMemManager().AllocatePhysicalPage(&uiFreePageNo), MEM_SUCCESS, ProcessAllocator_FAILURE) ;
		KC::MMemManager().GetKernelProcessStackPTEBase()[iStackBlockID * PROCESS_KERNEL_STACK_PAGES + i] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x3 ;
	}

	return ProcessAllocator_SUCCESS ;
}

void
ProcessAllocator_DeAllocateAddressSpaceForKernel(ProcessAddressSpace* processAddressSpace)
{
	unsigned i ;
	const int iStackBlockID = processAddressSpace->iKernelStackBlockID ;

	for(i = 0; i < PROCESS_KERNEL_STACK_PAGES; i++)
	{
		KC::MMemManager().DeAllocatePhysicalPage(
		(KC::MMemManager().GetKernelProcessStackPTEBase()[iStackBlockID * PROCESS_KERNEL_STACK_PAGES + i] & 0xFFFFF000) / PAGE_SIZE) ;
	}

	KC::MMemManager().FreeKernelProcessStackBlock(iStackBlockID) ;
}

byte ProcessAllocator_AllocatePagesForDLL(unsigned uiNoOfPagesForDLL, ProcessSharedObjectList* pProcessSharedObjectList)
{
	unsigned uiFreePageNo ;
	unsigned i ;

	pProcessSharedObjectList->uiAllocatedPageNumbers = (unsigned*)DMM_AllocateForKernel(sizeof(unsigned) * uiNoOfPagesForDLL) ;
	for(i = 0; i < uiNoOfPagesForDLL; i++)
	{
		RETURN_X_IF_NOT(KC::MMemManager().AllocatePhysicalPage(&uiFreePageNo), MEM_SUCCESS, ProcessAllocator_FAILURE) ;
		pProcessSharedObjectList->uiAllocatedPageNumbers[i] = uiFreePageNo ;	
	}

	pProcessSharedObjectList->uiNoOfPages = uiNoOfPagesForDLL ;
	return ProcessAllocator_SUCCESS ;
}

