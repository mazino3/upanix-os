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
#include <PCIBusHandler.h>
#include <DMM.h>
#include <MemManager.h>
#include <MemUtil.h>
#include <stdio.h>
#include <XHCIController.h>

unsigned XHCIController::_memMapBaseAddress = XHCI_MMIO_BASE_ADDR;

XHCIController::XHCIController(PCIEntry* pPCIEntry)
  : _pPCIEntry(pPCIEntry), _capReg(nullptr), _opReg(nullptr)
{
//	if(!pPCIEntry->BusEntity.NonBridge.bInterruptLine)
  //  throw upan::exception(XLOC, "XHCI device with no IRQ. Check BIOS/PCI settings!");

	unsigned uiIOAddr = pPCIEntry->BusEntity.NonBridge.uiBaseAddress0;
	printf("\n PCI BaseAddr: %x", uiIOAddr);

	uiIOAddr = uiIOAddr & PCI_ADDRESS_MEMORY_32_MASK;
	unsigned ioSize = pPCIEntry->GetPCIMemSize(0);
	printf(", Raw MMIO BaseAddr: %x, IOSize: %d", uiIOAddr, ioSize);

  const unsigned availableMemMapSize = XHCI_MMIO_BASE_ADDR_END - _memMapBaseAddress;
	if(ioSize > availableMemMapSize)
    throw upan::exception(XLOC, "XHCI IO Size is %x > greater than available size %x !", ioSize, availableMemMapSize);

  unsigned pagesToMap = ioSize / PAGE_SIZE;
  if(ioSize % PAGE_SIZE)
    ++pagesToMap;
	unsigned uiPDEAddress = MEM_PDBR ;
  const unsigned uiMappedIOAddr = _memMapBaseAddress + (uiIOAddr % PAGE_SIZE) - GLOBAL_DATA_SEGMENT_BASE;
  for(unsigned i = 0; i < pagesToMap; ++i)
  {
  	unsigned uiPDEIndex = ((_memMapBaseAddress >> 22) & 0x3FF) ;
	  unsigned uiPTEIndex = ((_memMapBaseAddress >> 12) & 0x3FF) ;
	  unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;
    // This page is a Read Only area for user process. 0x5 => 101 => User Domain, Read Only, Present Bit
    ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = (uiIOAddr & 0xFFFFF000) | 0x5 ;
    if(MemManager::Instance().MarkPageAsAllocated(uiIOAddr / PAGE_SIZE) != Success)
    {
    }

    _memMapBaseAddress += PAGE_SIZE;
    uiIOAddr += PAGE_SIZE;
  }

	Mem_FlushTLB();

	_capReg = (XHCICapRegister*)uiMappedIOAddr;
	_opReg = (XHCIOpRegister*)(uiMappedIOAddr + _capReg->CapLength());

  _capReg->Print();
	printf("\n Bus: %d, Dev: %d, Func: %d", pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction);
	/* Enable busmaster */
	unsigned short usCommand;
	pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
	printf("\n CurVal of PCI_COMMAND: %x", usCommand);
	pPCIEntry->WritePCIConfig(PCI_COMMAND, 2, usCommand | PCI_COMMAND_IO | PCI_COMMAND_MASTER);
	printf(" -> After Bus Master Enable, PCI_COMMAND: %x", usCommand);

  if(!_opReg->IsHCReady())
  {
    ProcessManager::Instance().Sleep(100);
    if(!_opReg->IsHCReady())
      throw upan::exception(XLOC, "HC is not ready yet!");
  }

  //program Max slots
  _opReg->MaxSlotsEnabled(_capReg->MaxSlots());

  //program device context base address pointer
  unsigned deviceContextTable = MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE;
  memset((void*)(deviceContextTable - GLOBAL_DATA_SEGMENT_BASE), 0, PAGE_SIZE);
  _opReg->SetDCBaap(deviceContextTable);
}
