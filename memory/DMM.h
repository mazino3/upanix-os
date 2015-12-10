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
#ifndef _DMM_H_
#define _DMM_H_

#include <Global.h>
#include <MemConstants.h>
#include <Atomic.h>
#include <ProcessManager.h>

#define NULL 0x0

#define DMM_SUCCESS				0
#define DMM_BAD_DEALLOC			1
#define DMM_BAD_ALIGN			2
#define DMM_FAILURE				3

typedef struct
{
	unsigned uiAllocatedAddress ;
	unsigned uiReturnAddress ;
	unsigned uiSize ;
  union
  {
  	unsigned uiNextAUTAddress;
    unsigned uiByteStuffForAlign;
  };
} PACKED AllocationUnitTracker ; // AUT

unsigned DMM_Allocate(ProcessAddressSpace* processAddressSpace, unsigned uiSizeInBytes) ;
unsigned DMM_AllocateAlign(ProcessAddressSpace* processAddressSpace, unsigned uiSizeInBytes, unsigned uiAlignNumber) ;

unsigned DMM_AllocateForKernel(unsigned uiSizeInBytes) ;
unsigned DMM_AllocateAlignForKernel(unsigned uiSizeInBytes, unsigned uiAlignNumber) ;

byte DMM_DeAllocate(ProcessAddressSpace* processAddressSpace, unsigned uiAddress) ;
byte DMM_DeAllocateForKernel(unsigned uiAddress) ;

byte DMM_GetAllocSize(ProcessAddressSpace* processAddressSpace, unsigned uiAddress, int* iSize);
byte DMM_GetAllocSizeForKernel(unsigned uiAddress, int* iSize);
void DMM_DeAllocatePhysicalPages(ProcessAddressSpace* processAddressSpace);

unsigned DMM_KernelHeapAllocSize() ;
unsigned DMM_GetKernelHeapSize() ; ;

#endif
