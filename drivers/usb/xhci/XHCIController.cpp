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
  printf("\n Total pages to Map: %d", pagesToMap);
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

  PerformBiosToOSHandoff(uiMappedIOAddr);

  if(!_opReg->IsHCReady())
  {
    ProcessManager::Instance().Sleep(100);
    if(!_opReg->IsHCReady())
      throw upan::exception(XLOC, "HC is not ready yet!");
  }

  _opReg->HCReset();

  //program Max slots
  _opReg->MaxSlotsEnabled(_capReg->MaxSlots());

  //program device context base address pointer
  unsigned deviceContextTable = MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE;
  memset((void*)(deviceContextTable - GLOBAL_DATA_SEGMENT_BASE), 0, PAGE_SIZE);
  _opReg->SetDCBaap(deviceContextTable);

  //Setup Command Ring
  _cmdManager = new CommandManager(*_capReg, *_opReg);

  //Start HC
  _opReg->DisableHCInterrupt();
  _opReg->Run();
  ProcessManager::Instance().Sleep(100);
  if(!_opReg->IsHCRunning() || _opReg->IsHCHalted())
    throw upan::exception(XLOC, "Failed to start XHCI HC");
  Probe();
}

void XHCIController::PerformBiosToOSHandoff(unsigned base)
{
	unsigned ecpOffset = _capReg->ECPOffset();

	if(!ecpOffset)
  {
    printf("\nXHCI System does not support Extended Capabilities. Cannot perform Bios Handoff");
    return;
  }
  printf("\n Trying to perform complete handoff of XHCI Controller from BIOS to OS - ECP Offset: 0x%x", ecpOffset);

  while(true)
  {
    unsigned& xCapReg = *(unsigned*)(base + ecpOffset);
    unsigned capId = xCapReg & 0xFF;
    if(!capId)
      throw upan::exception(XLOC, "Invalid Xtended CapID 0!");
    if(capId != 1)
    {
      unsigned nextOffset = (xCapReg >> 6) & 0x3FC;
      if(!nextOffset)
      {
        printf("\n Didn't find LEGSUP Capability");
        return;
      }
      ecpOffset += nextOffset;
    }
    else
    {
      printf("\n USB XHCI LEGSUP: 0x%x", xCapReg);
      if((xCapReg & (1 << 24)) == 0x1)
      {
        printf(", XHCI Controller is already owned by OS. No need for Handoff");
        return;
      }

      xCapReg |= ( 1 << 24 );
      if(!ProcessManager::Instance().ConditionalWait(&xCapReg, 24, true))
        throw upan::exception(XLOC, "BIOS to OS Handoff failed - HC Owned bit is stil not set");
      if(!ProcessManager::Instance().ConditionalWait(&xCapReg, 16, false))
        throw upan::exception(XLOC, "BIOS to OS Handoff failed - Bios Owned bit is still set");

      printf(", New USB XHCI LEGSUP: 0x%x", xCapReg) ;
      break;
    }
  }
}

CommandManager::CommandManager(XHCICapRegister& creg, XHCIOpRegister& oreg)
  : _pcs(true), _ring(nullptr), _capReg(creg), _opReg(oreg)
{
  _ring = new ((void*)DMM_AllocateAlignForKernel(sizeof(Ring), 64))Ring();

  LinkTRB link(_ring->_link, true);
  link.SetLinkAddr((uint64_t)&_ring->_cmd);
  link.SetToggleBit(true);

  _opReg.SetCommandRingPointer((uint64_t)_ring); 
}

void XHCIController::Probe()
{
	printf("\n Setup Ports") ;
  unsigned uiNoOfPorts = _capReg->MaxPorts();
	printf("-> NumPorts = %d", uiNoOfPorts) ;
  if(_capReg->IsPPC())
		printf("-> Software PortPowerCntrl") ;
	else
		printf("-> Hardware PortPowerCtrl") ;
	for(unsigned i = 0; i < uiNoOfPorts; i++)
	{
    auto& port = _opReg->PortRegisters()[i];
		printf("\n Setup Port: %d -> ", i);
    port.Print();
		if(_capReg->IsPPC())
      port.PowerOn();

    if(!port.IsPowerOn())
    {
      printf("\n Port %d is not Powered On!", i);
      continue;
    }

    if(!port.IsEnabled())
    {
//      try
//      {
//        port.Reset();
//        if(!port.IsEnabled())
//        {
          printf("\n Port %d is not enabled!", i);
          continue;
//        }
//      }
//      catch(const upan::exception& e)
//      {
//        printf("\n Reset failed: %s", e.Error().c_str());
//        continue;
//      }
    }

    if(port.IsDeviceConnected())
      printf("\n Device connected to Port %d", i);
    else
      printf("\n No Device connected to Port %d", i);

//		if((*pPort & 0x1))
//			printf("-> High Speed Dev Cncted") ;
//		printf("-> New value: %x", i, *pPort) ;
//
//    auto driver = USBController::Instance().FindDriver(new EHCIDevice(*this));
//    if(driver)
//      printf("\n'%s' driver found for the USB Device", driver->Name().c_str());
//    else
//      printf("\nNo Driver found for this USB device") ;
	}
}
