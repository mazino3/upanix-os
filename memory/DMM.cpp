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
#include <DMM.h>
#include <MemManager.h>
#include <Display.h>
#include <MOSMain.h>
#include <stdio.h>

unsigned DMM_uiTotalKernelAllocation = 0 ;

#define VIRTUAL_ALLOCATED_ADDRESS(RealAddress) ((RealAddress + GLOBAL_DATA_SEGMENT_BASE) - PROCESS_BASE)

#define REAL_ALLOCATED_ADDRESS(VirtualAddress) ((VirtualAddress + PROCESS_BASE) - GLOBAL_DATA_SEGMENT_BASE)

/*************** static (private) functions ********************/

static unsigned DMM_GetByteStuffForAlign(unsigned uiAddress, unsigned uiAlignNumber)
{
	unsigned uiByteStuffForAlign = 0 ;

	if(uiAlignNumber != 0)
	{
		unsigned uiAlignMod = uiAddress % uiAlignNumber ;
		if(uiAlignMod != 0)
			uiByteStuffForAlign = uiAlignNumber - uiAlignMod ;
	}

	return uiByteStuffForAlign ;
}

static byte DMM_CheckAlignNumber(unsigned uiAlignNumber)
{
	static unsigned ALIGN[] = { 0, 1, 2, 4, 8, 16, 32, 64, 99 } ;
	unsigned i ;
	for(i = 0; ALIGN[i] != 99; i++)
		if(uiAlignNumber == ALIGN[i])
			return DMM_SUCCESS ;

	KC::MDisplay().Number("\n Align Number: ", uiAlignNumber) ;
	KC::MDisplay().Message(" is not supported.", Display::WHITE_ON_BLACK()) ;

	return DMM_FAILURE ;
}

static unsigned DMM_AllocateAlignForKernel_Act(unsigned uiSizeInBytes, unsigned uiAlignNumber)
{
	if(DMM_CheckAlignNumber(uiAlignNumber) != DMM_SUCCESS)
		return DMM_BAD_ALIGN ;

	unsigned uiHeapStartAddress ;
	unsigned* uiAUTAddress ;

	uiAUTAddress = MemManager::Instance().GetKernelAUTAddress() ;

	uiHeapStartAddress = MemManager::Instance().GetKernelHeapStartAddr() - GLOBAL_DATA_SEGMENT_BASE ;
	AllocationUnitTracker *aut, *prevAut ;
	unsigned uiAddress ;
	
	// TODO: Limit Check on Allocated Memory i.e, Allocation should be limited to either
	// RAM Size OR 4GB if virtual memory management is implemented

	// Dedicated Head Node. This will avoid Back Loop at Head
	if(*uiAUTAddress == NULL)
	{
		aut = (AllocationUnitTracker*)(uiHeapStartAddress) ;

		aut->uiAllocatedAddress = uiHeapStartAddress ;
		aut->uiSize = 0 + sizeof(AllocationUnitTracker) ;

		aut->uiNextAUTAddress = NULL ;
		*uiAUTAddress = uiHeapStartAddress ;
	}

	unsigned uiByteStuffForAlign = 0;

	aut = prevAut = (AllocationUnitTracker*)(*uiAUTAddress) ;
	byte bStop = false ;
	for(uiAddress = uiHeapStartAddress; ;)
	{
		for(;;)
		{
			if(bStop == true || aut == NULL)
			{
				if(aut == NULL)
				{
					uiByteStuffForAlign = DMM_GetByteStuffForAlign(uiAddress+sizeof(AllocationUnitTracker), uiAlignNumber) ;
				}

				aut = (AllocationUnitTracker*)(uiAddress) ;

				aut->uiAllocatedAddress = uiAddress ;
				aut->uiReturnAddress = uiAddress + sizeof(AllocationUnitTracker) + uiByteStuffForAlign ;
				aut->uiSize = uiSizeInBytes + sizeof(AllocationUnitTracker) + uiByteStuffForAlign ;

				if(prevAut != NULL)
				{
					aut->uiNextAUTAddress = prevAut->uiNextAUTAddress ;
					prevAut->uiNextAUTAddress = (unsigned)aut ;
				}
				else
				{
					aut->uiNextAUTAddress = NULL ;
					*uiAUTAddress = uiAddress ;
				}

				DMM_uiTotalKernelAllocation += aut->uiSize ;

				return aut->uiReturnAddress ;
			}
		
			uiByteStuffForAlign = DMM_GetByteStuffForAlign(uiAddress + sizeof(AllocationUnitTracker), uiAlignNumber) ;
			__volatile__ unsigned uiAllocSize = uiSizeInBytes + sizeof(AllocationUnitTracker) + uiByteStuffForAlign ;

			if(!(
			((uiAddress < aut->uiAllocatedAddress) && ((uiAddress + uiAllocSize) < aut->uiAllocatedAddress))
				||
			((uiAddress >= (aut->uiAllocatedAddress + aut->uiSize)) && ((uiAddress + uiAllocSize) >= (aut->uiAllocatedAddress + aut->uiSize)))
			))
			{
				break ;
			}
			else
			{
				bStop = true ;
		//		prevAut = aut ;
		//		aut = (AllocationUnitTracker*)(aut->uiNextAUTAddress) ;
			}
		}
		uiAddress = aut->uiAllocatedAddress + aut->uiSize ;
		prevAut = aut ;
		aut = (AllocationUnitTracker*)(aut->uiNextAUTAddress) ;
	}

	return NULL ;
}

static byte DMM_DeAllocateForKernel_Act(unsigned uiAddress)
{
	unsigned* uiAUTAddress ;
	unsigned uiHeapStartAddress = MemManager::Instance().GetKernelHeapStartAddr() - GLOBAL_DATA_SEGMENT_BASE ;

	if(uiAddress == NULL)
		return DMM_SUCCESS ;

	if(uiAddress == uiHeapStartAddress)
		return DMM_BAD_DEALLOC ;

	uiAUTAddress = MemManager::Instance().GetKernelAUTAddress() ;

	AllocationUnitTracker* curAUT = (AllocationUnitTracker*)(*uiAUTAddress) ;
	AllocationUnitTracker* prevAUT = NULL ;

	while(curAUT != NULL)
	{
		if(curAUT->uiReturnAddress == uiAddress)
		{
			if(prevAUT == NULL)
				*uiAUTAddress = curAUT->uiNextAUTAddress ;
			else
				prevAUT->uiNextAUTAddress = curAUT->uiNextAUTAddress ;

			DMM_uiTotalKernelAllocation -= curAUT->uiSize ;
			return DMM_SUCCESS ;
		}

		prevAUT = curAUT ;
		curAUT = (AllocationUnitTracker*)(curAUT->uiNextAUTAddress) ;
	}

	printf("\n Bad DeAlloc request: %u", uiAddress) ;
	while(1) ;
	return DMM_BAD_DEALLOC ;
}


/***************************************************************/

unsigned DMM_Allocate(ProcessAddressSpace* processAddressSpace, unsigned uiSizeInBytes)
{
	return DMM_AllocateAlign(processAddressSpace, uiSizeInBytes, 0) ;
}

unsigned DMM_AllocateAlign(ProcessAddressSpace* processAddressSpace, unsigned uiSizeInBytes, unsigned uiAlignNumber)
{
	if(DMM_CheckAlignNumber(uiAlignNumber) != DMM_SUCCESS)
		return DMM_BAD_ALIGN ;
	
	ProcessManager_SetDMMFlag(ProcessManager_iCurrentProcessID, true) ;

	__volatile__ unsigned uiHeapStartAddress = PROCESS_HEAP_START_ADDRESS - GLOBAL_DATA_SEGMENT_BASE ;

	AllocationUnitTracker *aut, *prevAut ;
	unsigned uiAddress ;

	// TODO: Limit Check on Allocated Memory i.e, Allocation should be limited to either
	// RAM Size OR 4GB if virtual memory management is implemented
	
	// Dedicated Head Node. This will avoid Back Loop at Head
	if(processAddressSpace->uiAUTAddress == NULL)
	{
		aut = (AllocationUnitTracker*)(uiHeapStartAddress) ;

		aut->uiAllocatedAddress = uiHeapStartAddress ;
		aut->uiSize = 0 + sizeof(AllocationUnitTracker) ;

		aut->uiNextAUTAddress = NULL ;
		processAddressSpace->uiAUTAddress = uiHeapStartAddress ;
	}

	unsigned uiByteStuffForAlign = 0;

	aut = prevAut = (AllocationUnitTracker*)(processAddressSpace->uiAUTAddress) ;
	byte bStop = false ;
	for(uiAddress = uiHeapStartAddress; ;)
	{
		for(;;)
		{
			if(bStop == true || aut == NULL)
			{
				if(aut == NULL)
				{
					uiByteStuffForAlign = DMM_GetByteStuffForAlign(uiAddress+sizeof(AllocationUnitTracker), uiAlignNumber) ;
				}

				aut = (AllocationUnitTracker*)(uiAddress) ;

				aut->uiAllocatedAddress = uiAddress ;
				aut->uiReturnAddress = uiAddress + sizeof(AllocationUnitTracker) + uiByteStuffForAlign ;
				aut->uiSize = uiSizeInBytes + sizeof(AllocationUnitTracker) + uiByteStuffForAlign ;

				if(prevAut != NULL)
				{
					aut->uiNextAUTAddress = prevAut->uiNextAUTAddress ;
					prevAut->uiNextAUTAddress = (unsigned)aut ;
				}
				else
				{
					aut->uiNextAUTAddress = NULL ;
					processAddressSpace->uiAUTAddress = uiAddress ;
				}

				//Make sure that all pages are allocated in the requested mem block
				unsigned addr = uiAddress + sizeof(AllocationUnitTracker) ;
				__volatile int x ;
				while(addr < (uiAddress + aut->uiSize))
				{
					x = ((char*)addr)[0] ; // A read would cause a page fault
					addr += PAGE_SIZE ;
				}
				x = ((char*)(uiAddress + aut->uiSize - 1))[0] ;

				ProcessManager_SetDMMFlag(ProcessManager_iCurrentProcessID, false) ;
				
				return VIRTUAL_ALLOCATED_ADDRESS(aut->uiReturnAddress) ;
			}

			uiByteStuffForAlign = DMM_GetByteStuffForAlign(uiAddress + sizeof(AllocationUnitTracker), uiAlignNumber) ;
			unsigned uiAllocSize = uiSizeInBytes + sizeof(AllocationUnitTracker) + uiByteStuffForAlign ;

			if(!(
			((uiAddress < aut->uiAllocatedAddress) && ((uiAddress + uiAllocSize) < aut->uiAllocatedAddress))
				||
			((uiAddress >= (aut->uiAllocatedAddress + aut->uiSize)) && ((uiAddress + uiAllocSize) >= (aut->uiAllocatedAddress + aut->uiSize)))
			))
			{
				break ;
			}
			else
			{
				bStop = true ;
		//		prevAut = aut ;
		//		aut = (AllocationUnitTracker*)(aut->uiNextAUTAddress) ;
			}
		}
		uiAddress = aut->uiAllocatedAddress + aut->uiSize ;
		prevAut = aut ;
		aut = (AllocationUnitTracker*)(aut->uiNextAUTAddress) ;
	}

	ProcessManager_SetDMMFlag(ProcessManager_iCurrentProcessID, false) ;
	return NULL ;
}

unsigned DMM_AllocateForKernel(unsigned uiSizeInBytes)
{
	return DMM_AllocateAlignForKernel(uiSizeInBytes, 0) ;
}

unsigned DMM_AllocateAlignForKernel(unsigned uiSizeInBytes, unsigned uiAlignNumber)
{
	MOSMain_GetDMMMutex().Lock() ;

	unsigned uiAddr = DMM_AllocateAlignForKernel_Act(uiSizeInBytes, uiAlignNumber) ;

	MOSMain_GetDMMMutex().UnLock() ;

	return uiAddr ;
}

byte DMM_DeAllocate(ProcessAddressSpace* processAddressSpace, unsigned uiAddress)
{
	uiAddress = REAL_ALLOCATED_ADDRESS(uiAddress) ;
	unsigned uiHeapStartAddress = PROCESS_HEAP_START_ADDRESS - GLOBAL_DATA_SEGMENT_BASE ;

	if(uiAddress == NULL)
		return DMM_SUCCESS ;

	if(uiAddress == uiHeapStartAddress)
		return DMM_BAD_DEALLOC ;
	
	AllocationUnitTracker* curAUT = (AllocationUnitTracker*)processAddressSpace->uiAUTAddress ;
	AllocationUnitTracker* prevAUT = NULL ;

	while(curAUT != NULL)
	{
		if(curAUT->uiReturnAddress == uiAddress)
		{
			if(prevAUT == NULL)
				processAddressSpace->uiAUTAddress = curAUT->uiNextAUTAddress ;
			else
				prevAUT->uiNextAUTAddress = curAUT->uiNextAUTAddress ;

			return DMM_SUCCESS ;
		}

		prevAUT = curAUT ;
		curAUT = (AllocationUnitTracker*)(curAUT->uiNextAUTAddress) ;
	}

	return DMM_BAD_DEALLOC ;
}

byte DMM_GetAllocSize(ProcessAddressSpace* processAddressSpace, unsigned uiAddress, int* iSize)
{
	uiAddress = REAL_ALLOCATED_ADDRESS(uiAddress) ;

	AllocationUnitTracker* curAUT = (AllocationUnitTracker*)processAddressSpace->uiAUTAddress ;

	while(curAUT != NULL)
	{
		if(curAUT->uiReturnAddress == uiAddress)
		{
			*iSize = curAUT->uiSize - (curAUT->uiReturnAddress - curAUT->uiAllocatedAddress) ;
			return DMM_SUCCESS ;
		}

		curAUT = (AllocationUnitTracker*)(curAUT->uiNextAUTAddress) ;
	}

	return DMM_BAD_DEALLOC ;
}

byte DMM_GetAllocSizeForKernel(unsigned uiAddress, int* iSize)
{
	MOSMain_GetDMMMutex().Lock();
	unsigned* uiAUTAddress = MemManager::Instance().GetKernelAUTAddress();
	unsigned uiHeapStartAddress = MemManager::Instance().GetKernelHeapStartAddr() - GLOBAL_DATA_SEGMENT_BASE;

	AllocationUnitTracker* curAUT = (AllocationUnitTracker*)(*uiAUTAddress);

	while(curAUT != NULL)
	{
		if(curAUT->uiReturnAddress == uiAddress)
		{
			*iSize = curAUT->uiSize - (curAUT->uiReturnAddress - curAUT->uiAllocatedAddress) ;
			return DMM_SUCCESS ;
		}

		curAUT = (AllocationUnitTracker*)(curAUT->uiNextAUTAddress) ;
	}

	MOSMain_GetDMMMutex().UnLock();
	return DMM_BAD_DEALLOC ;
}

byte DMM_DeAllocateForKernel(unsigned uiAddress)
{
	MOSMain_GetDMMMutex().Lock() ;

	unsigned bStatus = DMM_DeAllocateForKernel_Act(uiAddress) ;

	MOSMain_GetDMMMutex().UnLock() ;

	return bStatus ;
}
	
void DMM_DeAllocatePhysicalPages(ProcessAddressSpace* processAddressSpace)
{
	unsigned uiPDEAddress = processAddressSpace->taskState.CR3_PDBR ;
	unsigned uiPDEIndex = ((PROCESS_HEAP_START_ADDRESS >> 22) & 0x3FF) ;
	unsigned uiPTEIndex = ((PROCESS_HEAP_START_ADDRESS >> 12) & 0x3FF) ;

	unsigned uiPTEAddress;
	unsigned uiPageNumber;
	unsigned uiPresentBit;
	unsigned uiAddr;
	
	unsigned i, j;
	for(i = uiPDEIndex; i < PAGE_TABLE_ENTRIES; i++)
	{
		uiAddr = ((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[i];
		uiPresentBit = uiAddr & 0x1;
		uiPTEAddress = uiAddr & 0xFFFFF000;

		if(uiPresentBit && uiPTEAddress != NULL)
		{
			for(j = uiPTEIndex; j < PAGE_TABLE_ENTRIES; j++)
			{
				uiAddr = ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[j];
				uiPresentBit =  uiAddr & 0x1;
				uiPageNumber = (uiAddr & 0xFFFFF000) / PAGE_SIZE;
				
				if(uiPresentBit && uiPageNumber != 0)
				{
					MemManager::Instance().DeAllocatePhysicalPage(uiPageNumber);
				}
				else
					break;
			}
		
			uiPageNumber = uiPTEAddress / PAGE_SIZE;
			MemManager::Instance().DeAllocatePhysicalPage(uiPageNumber);
			
			uiPTEIndex = 0;
		}
		else
			break;
	}
}

unsigned DMM_KernelHeapAllocSize()
{
	unsigned uiHeapStartAddress = MemManager::Instance().GetKernelHeapStartAddr() - GLOBAL_DATA_SEGMENT_BASE ;
	AllocationUnitTracker *aut ;
	unsigned uiSize = 0 ;

	for(aut = (AllocationUnitTracker*)uiHeapStartAddress; aut != NULL; aut = (AllocationUnitTracker*)(aut->uiNextAUTAddress))
	{
		uiSize += aut->uiSize;
	}

	return uiSize ;
}

unsigned DMM_GetKernelHeapSize()
{
	return DMM_uiTotalKernelAllocation ;
}
