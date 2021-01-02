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
#include <KernelProcess.h>
#include <ProcessEnv.h>
#include <UserManager.h>
#include <ProcessManager.h>

KernelProcess::KernelProcess(const upan::string& name, uint32_t taskAddress, int parentID, bool isFGProcess, uint32_t param1, uint32_t param2)
    : Process(name, parentID, isFGProcess) {
  _mainThreadID = _processID;
  ProcessEnv_InitializeForKernelProcess() ;
  _processBase = GLOBAL_DATA_SEGMENT_BASE;
  const uint32_t uiStackAddress = AllocateAddressSpace();
  const uint32_t uiStackTop = uiStackAddress - GLOBAL_DATA_SEGMENT_BASE + (PROCESS_KERNEL_STACK_PAGES * PAGE_SIZE) - 1;
  _taskState.BuildForKernel(taskAddress, uiStackTop, param1, param2);
  _processLDT.BuildForKernel();
  _userID = ROOT_USER_ID ;
}

uint32_t KernelProcess::AllocateAddressSpace() {
  const int iStackBlockID = MemManager::Instance().GetFreeKernelProcessStackBlockID() ;

  if(iStackBlockID < 0)
    throw upan::exception(XLOC, "No free stack blocks available for kernel process");

  kernelStackBlockId = iStackBlockID ;

  for(int i = 0; i < PROCESS_KERNEL_STACK_PAGES; i++)
  {
    uint32_t uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
    MemManager::Instance().GetKernelProcessStackPTEBase()[iStackBlockID * PROCESS_KERNEL_STACK_PAGES + i] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x3 ;
  }
  Mem_FlushTLB();

  return KERNEL_PROCESS_PDE_ID * PAGE_TABLE_ENTRIES * PAGE_SIZE + iStackBlockID * PROCESS_KERNEL_STACK_PAGES * PAGE_SIZE;
}

void KernelProcess::DeAllocateResources() {
  for(int i = 0; i < PROCESS_KERNEL_STACK_PAGES; i++) {
    MemManager::Instance().DeAllocatePhysicalPage(
        (MemManager::Instance().GetKernelProcessStackPTEBase()[kernelStackBlockId * PROCESS_KERNEL_STACK_PAGES + i] & 0xFFFFF000) / PAGE_SIZE) ;
  }
  Mem_FlushTLB();

  MemManager::Instance().FreeKernelProcessStackBlock(kernelStackBlockId) ;
}