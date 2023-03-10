/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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
#include <UpanixMain.h>
#include <stdio.h>
#include <exception.h>
#include <ProcessManager.h>
#include <Bit.h>

unsigned DMM_uiTotalKernelAllocation = 0 ;

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

void DMM_CheckAlignNumber(unsigned uiAlignNumber)
{
  if(uiAlignNumber == 0)
    return;
  if(Bit::IsPowerOfTwo(uiAlignNumber))
    return;
  throw upan::exception(XLOC, "%u is not 2^ power aligned address", uiAlignNumber);
}

static uint32_t calculateCheckSum(AllocationUnitTracker& aut) {
  return aut.uiAllocatedAddress ^ aut.uiSize ^ aut.uiReturnAddress ^ aut.uiNextAUTAddress;
}

static void updateCheckSum(AllocationUnitTracker& aut) {
  aut.uiCheckSum = calculateCheckSum(aut);
}

/***************************************************************/

// old allocation algorithm which was maintaining a list of allocated chunks was 40 times slower
// than the current algorithm which maintains a list of free chunks
unsigned DMM_Allocate(Process* processAddressSpace, unsigned uiSizeInBytes, unsigned uiAlignNumber)
{
  upan::mutex_guard g(processAddressSpace->heapMutex().value());

  DMM_CheckAlignNumber(uiAlignNumber);

  ProcessManager::Instance().SetDMMFlag(ProcessManager::GetCurrentProcessID(), true) ;

  __volatile__ unsigned uiHeapStartAddress = PROCESS_HEAP_START_ADDRESS - GLOBAL_DATA_SEGMENT_BASE ;

  AllocationUnitTracker *aut, *prevAut ;

  // TODO: Limit Check on Allocated Memory i.e, Allocation should be limited to either
  // RAM Size OR 4GB if virtual memory management is implemented

  // Dedicated Head Node. This will avoid Back Loop at Head
  if(processAddressSpace->getAUTAddress() == NULL)
  {
    aut = (AllocationUnitTracker*)(uiHeapStartAddress) ;

    aut->uiAllocatedAddress = uiHeapStartAddress ;
    aut->uiSize = PROCESS_HEAP_SIZE ;
    aut->uiReturnAddress = NULL;
    aut->uiNextAUTAddress = NULL ;
    processAddressSpace->setAUTAddress(uiHeapStartAddress);
  }

  aut = (AllocationUnitTracker*)(processAddressSpace->getAUTAddress()) ;
  prevAut = NULL;
  while(aut != NULL)
  {
    unsigned uiAddress = aut->uiAllocatedAddress;
    unsigned uiMaxSize = aut->uiSize;
    unsigned uiNextAUTAddress = aut->uiNextAUTAddress;
    unsigned uiByteStuffForAlign = DMM_GetByteStuffForAlign(uiAddress + sizeof(AllocationUnitTracker), uiAlignNumber);
    unsigned uiSize = uiSizeInBytes + sizeof(AllocationUnitTracker) + uiByteStuffForAlign;

    if(uiSize <= uiMaxSize)
    {
      AllocationUnitTracker* allocAUT = (AllocationUnitTracker*)(uiAddress + uiByteStuffForAlign);
      allocAUT->uiAllocatedAddress = uiAddress;
      allocAUT->uiReturnAddress = uiAddress + sizeof(AllocationUnitTracker) + uiByteStuffForAlign;
      allocAUT->uiSize = uiSize;
      allocAUT->uiByteStuffForAlign = uiByteStuffForAlign;

      unsigned uiRemaining = uiMaxSize - uiSize;
      if(uiRemaining > (sizeof(AllocationUnitTracker) + 1))
      {
        AllocationUnitTracker* freeAUT = (AllocationUnitTracker*)(uiAddress + uiSize);
        freeAUT->uiAllocatedAddress = uiAddress + uiSize;
        freeAUT->uiReturnAddress = NULL;
        freeAUT->uiSize = uiRemaining;
        freeAUT->uiNextAUTAddress = uiNextAUTAddress;

        if(prevAut == NULL)
          processAddressSpace->setAUTAddress(freeAUT->uiAllocatedAddress);
        else
          prevAut->uiNextAUTAddress = freeAUT->uiAllocatedAddress;
      }
      else
      {
        allocAUT->uiSize += uiRemaining;
        if(prevAut == NULL)
          processAddressSpace->setAUTAddress(uiNextAUTAddress);
        else
          prevAut->uiNextAUTAddress = uiNextAUTAddress;
      }

      //Make sure that all pages are allocated in the requested mem block
      unsigned addr = uiAddress + sizeof(AllocationUnitTracker) ;
      UNUSED __volatile__ int x;
      while(addr < (uiAddress + aut->uiSize))
      {
        x = ((char*)addr)[0] ; // A read would cause a page fault
        addr += PAGE_SIZE ;
      }
      x = ((char*)(uiAddress + aut->uiSize - 1))[0] ;

      ProcessManager::Instance().SetDMMFlag(ProcessManager::GetCurrentProcessID(), false) ;

      return PROCESS_VIRTUAL_ALLOCATED_ADDRESS(aut->uiReturnAddress) ;
    }

    prevAut = aut;
    aut = (AllocationUnitTracker*)uiNextAUTAddress;
  }
  throw upan::exception(XLOC, "out of memory!");
}

uint32_t dmm_alloc_count = 0;
unsigned DMM_AllocateForKernel(unsigned uiSizeInBytes, unsigned uiAlignNumber)
{
  IrqGuard g;
  ++dmm_alloc_count;
	DMM_CheckAlignNumber(uiAlignNumber);
	AllocationUnitTracker *aut, *prevAut;
	unsigned& uiAUTAddress = MemManager::Instance().GetKernelAUTAddress();

	// Dedicated Head Node. This will avoid Back Loop at Head
	if(uiAUTAddress == NULL) {
	  unsigned uiHeapStartAddress = MemManager::Instance().GetKernelHeapStartAddr();
		aut = (AllocationUnitTracker*)(uiHeapStartAddress);
    aut->uiAllocatedAddress = uiHeapStartAddress;
		aut->uiSize = MEM_KERNEL_HEAP_SIZE;
    aut->uiReturnAddress = NULL;
		aut->uiNextAUTAddress = NULL;
    updateCheckSum(*aut);
		uiAUTAddress = uiHeapStartAddress;
	}

	aut = (AllocationUnitTracker*)(uiAUTAddress);
  prevAut = NULL;
  while(aut != NULL) {
    unsigned uiAddress = aut->uiAllocatedAddress;
    unsigned uiMaxSize = aut->uiSize;
    unsigned uiNextAUTAddress = aut->uiNextAUTAddress;
    unsigned uiByteStuffForAlign = DMM_GetByteStuffForAlign(uiAddress + sizeof(AllocationUnitTracker), uiAlignNumber);
    unsigned uiSize = uiSizeInBytes + sizeof(AllocationUnitTracker) + uiByteStuffForAlign;
    const uint32_t calcCheckSum = calculateCheckSum(*aut);
    if (calcCheckSum != aut->uiCheckSum) {
      throw upan::exception(XLOC,"heap corrupted!");
    }
    if(uiSize <= uiMaxSize)
    {
      auto allocAUT = (AllocationUnitTracker*)(uiAddress + uiByteStuffForAlign);
      allocAUT->uiAllocatedAddress = uiAddress;
      allocAUT->uiReturnAddress = uiAddress + sizeof(AllocationUnitTracker) + uiByteStuffForAlign;
      allocAUT->uiSize = uiSize;
      allocAUT->uiByteStuffForAlign = uiByteStuffForAlign;

      unsigned uiRemaining = uiMaxSize - uiSize;
      if(uiRemaining > (sizeof(AllocationUnitTracker) + 1))
      {
        auto freeAUT = (AllocationUnitTracker*)(uiAddress + uiSize);
        freeAUT->uiAllocatedAddress = uiAddress + uiSize;
        freeAUT->uiReturnAddress = NULL;
        freeAUT->uiSize = uiRemaining;
        freeAUT->uiNextAUTAddress = uiNextAUTAddress;

        if(prevAut == NULL)
          uiAUTAddress = freeAUT->uiAllocatedAddress;
        else
          prevAut->uiNextAUTAddress = freeAUT->uiAllocatedAddress;
        updateCheckSum(*freeAUT);
      }
      else
      {
        allocAUT->uiSize += uiRemaining;
        if(prevAut == NULL)
          uiAUTAddress = uiNextAUTAddress;
        else
          prevAut->uiNextAUTAddress = uiNextAUTAddress;
      }
      if (prevAut != NULL) {
        updateCheckSum(*prevAut);
      }
      updateCheckSum(*allocAUT);
      return allocAUT->uiReturnAddress;
    }
    prevAut = aut;
    aut = (AllocationUnitTracker*)uiNextAUTAddress;
  }
  throw upan::exception(XLOC, "out of memory!");
}

byte DMM_DeAllocate(Process* processAddressSpace, unsigned uiAddress)
{
  upan::mutex_guard g(processAddressSpace->heapMutex().value());
  // do this before converting the address to real address (by adding PROCESS_BASE)
  if(uiAddress == NULL)
    return DMM_SUCCESS ;

  uiAddress = PROCESS_REAL_ALLOCATED_ADDRESS(uiAddress) ;
  unsigned uiHeapStartAddress = PROCESS_HEAP_START_ADDRESS - GLOBAL_DATA_SEGMENT_BASE ;

  if(uiAddress <= uiHeapStartAddress)
    return DMM_BAD_DEALLOC ;

  AllocationUnitTracker* freeAUT = (AllocationUnitTracker*)(uiAddress - sizeof(AllocationUnitTracker));
  unsigned uiAllocatedAddress = uiAddress - sizeof(AllocationUnitTracker) - freeAUT->uiByteStuffForAlign;
  unsigned uiSize = freeAUT->uiSize;
  freeAUT = (AllocationUnitTracker*)uiAllocatedAddress;
  freeAUT->uiAllocatedAddress = uiAllocatedAddress;
  freeAUT->uiReturnAddress = NULL;
  freeAUT->uiSize = uiSize;
  freeAUT->uiNextAUTAddress = processAddressSpace->getAUTAddress();
  processAddressSpace->setAUTAddress(uiAllocatedAddress);
  return DMM_SUCCESS;
}

byte DMM_GetAllocSize(unsigned uiAddress, int* iSize) {
  uiAddress = PROCESS_REAL_ALLOCATED_ADDRESS(uiAddress);
  unsigned uiHeapStartAddress = PROCESS_HEAP_START_ADDRESS - GLOBAL_DATA_SEGMENT_BASE ;

  if (uiAddress == NULL || uiAddress < sizeof(AllocationUnitTracker) || uiAddress == uiHeapStartAddress) {
    *iSize = 0;
    return DMM_FAILURE;
  }

  AllocationUnitTracker* aut = (AllocationUnitTracker *) (uiAddress - sizeof(AllocationUnitTracker));
  *iSize = aut->uiSize;
  return DMM_SUCCESS;
}

byte DMM_GetAllocSizeForKernel(unsigned uiAddress, int* iSize) {
  IrqGuard g;
  unsigned uiHeapStartAddress = MemManager::Instance().GetKernelHeapStartAddr();
  if (uiAddress == NULL || uiAddress < sizeof(AllocationUnitTracker) || uiAddress == uiHeapStartAddress) {
    *iSize = 0;
    return DMM_FAILURE;
  }

  AllocationUnitTracker* aut = (AllocationUnitTracker *) (uiAddress - sizeof(AllocationUnitTracker));
  *iSize = aut->uiSize;
  return DMM_SUCCESS;
}

byte DMM_DeAllocateForKernel(unsigned uiAddress)
{
  IrqGuard g;
	unsigned& uiAUTAddress = MemManager::Instance().GetKernelAUTAddress() ;
	unsigned uiHeapStartAddress = MemManager::Instance().GetKernelHeapStartAddr();

	if(uiAddress == NULL)
		return DMM_SUCCESS ;

	if(uiAddress <= uiHeapStartAddress)
		return DMM_BAD_DEALLOC ;

  AllocationUnitTracker* freeAUT = (AllocationUnitTracker*)(uiAddress - sizeof(AllocationUnitTracker));
  unsigned uiAllocatedAddress = uiAddress - sizeof(AllocationUnitTracker) - freeAUT->uiByteStuffForAlign;
  unsigned uiSize = freeAUT->uiSize;
  const uint32_t calcCheckSum = calculateCheckSum(*freeAUT);
  if (calcCheckSum != freeAUT->uiCheckSum) {
    throw upan::exception(XLOC,"bad address dealloc %x", uiAddress);
  }
  if (freeAUT->uiReturnAddress == NULL) {
    throw upan::exception(XLOC, "double delete %x", uiAddress);
  }
  freeAUT = (AllocationUnitTracker*)uiAllocatedAddress;
  freeAUT->uiAllocatedAddress = uiAllocatedAddress;
  freeAUT->uiReturnAddress = NULL;
  freeAUT->uiSize = uiSize;
  freeAUT->uiNextAUTAddress = uiAUTAddress;
  updateCheckSum(*freeAUT);
  uiAUTAddress = uiAllocatedAddress;
  return DMM_SUCCESS;
}
	
void DMM_DeAllocatePhysicalPages(Process* processAddressSpace)
{
	unsigned uiPDEAddress = processAddressSpace->pdbr();
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
	unsigned uiHeapStartAddress = MemManager::Instance().GetKernelHeapStartAddr();
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
