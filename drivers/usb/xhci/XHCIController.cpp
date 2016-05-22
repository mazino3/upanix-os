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
#include <KBDriver.h>
#include <XHCIController.h>

unsigned XHCIController::_memMapBaseAddress = XHCI_MMIO_BASE_ADDR;

XHCIController::XHCIController(PCIEntry* pPCIEntry)
  : _pPCIEntry(pPCIEntry), _capReg(nullptr), _opReg(nullptr),
    _legSupXCap(nullptr)
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

	printf("\n Bus: %d, Dev: %d, Func: %d", pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction);
	printf("\n Vendor: %x, Device: %x, Rev: %x", pPCIEntry->usVendorID, pPCIEntry->usDeviceID, pPCIEntry->bRevisionID);

	_capReg = (XHCICapRegister*)uiMappedIOAddr;
	_opReg = (XHCIOpRegister*)(uiMappedIOAddr + _capReg->CapLength());

  _capReg->Print();

  //Enable Ports for Intel PanthorPoint XHCI
  if(pPCIEntry->usVendorID == 0x8086
    && pPCIEntry->usDeviceID == 0x1E31)
  {
    unsigned portsAvailable;

    portsAvailable = 0xFFFFFFFF;
    /* Write USB3_PSSEN, the USB 3.0 Port SuperSpeed Enable
     * Register, to turn on SuperSpeed terminations for all
     * available ports.
     */
    pPCIEntry->WritePCIConfig(0xD8, 4, portsAvailable);
    pPCIEntry->ReadPCIConfig(0xD8, 4, &portsAvailable);
    printf("\n USB 3.0 ports that are now enabled: %x", portsAvailable);

    /* Write XUSB2PR, the xHC USB 2.0 Port Routing Register, to
     * switch the USB 2.0 power and data lines over to the xHCI
     * host.
     */
    portsAvailable = 0xFFFFFFFF;
    pPCIEntry->WritePCIConfig(0xD0, 4, portsAvailable);
    pPCIEntry->ReadPCIConfig(0xD0, 4, &portsAvailable);
    printf("\n USB 2.0 ports that are now enabled: %x", portsAvailable);
  }

	/* Enable busmaster */
	unsigned short usCommand;
	pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
	printf("\n CurVal of PCI_COMMAND: %x", usCommand);
	pPCIEntry->WritePCIConfig(PCI_COMMAND, 2, usCommand | PCI_COMMAND_MMIO | PCI_COMMAND_MASTER);
	pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
	printf(" -> After Bus Master Enable, PCI_COMMAND: %x", usCommand);

  if(!_opReg->IsHCReady())
    throw upan::exception(XLOC, "HC is not ready yet!");

  _opReg->Stop();
  _opReg->HCReset();

  if(!_opReg->IsHCReady())
    throw upan::exception(XLOC, "HC is not ready yet!");

  LoadXCaps(uiMappedIOAddr);
  PerformBiosToOSHandoff();

  //program Max slots
  _opReg->MaxSlotsEnabled(_capReg->MaxSlots());
  _opReg->SetDNCTRL(0x2);

  //program device context base address pointer
  unsigned deviceContextTable = MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE;
  memset((void*)(deviceContextTable - GLOBAL_DATA_SEGMENT_BASE), 0, PAGE_SIZE);
  _opReg->SetDCBaap(deviceContextTable);

  //Setup Command Ring
  _cmdManager = new CommandManager(*_capReg, *_opReg);

  //Setup Event Ring
  _eventManager = new EventManager(*_capReg, *_opReg);

  //Start HC
  _opReg->DisableHCInterrupt();
  _opReg->Run();
  ProcessManager::Instance().Sleep(100);
  if(!_opReg->IsHCRunning() || _opReg->IsHCHalted())
    throw upan::exception(XLOC, "Failed to start XHCI HC");
  _opReg->Print();
  Probe();
}

void XHCIController::LoadXCaps(unsigned base)
{
	unsigned ecpOffset = _capReg->ECPOffset();

	if(!ecpOffset)
  {
    printf("\nXHCI System does not support Extended Capabilities");
    return;
  }

  _legSupXCap = nullptr;
  _supProtoXCaps.clear();

  base += ecpOffset;
  while(true)
  {
    unsigned& xCapReg = *(unsigned*)(base);
    unsigned capId = xCapReg & 0xFF;
    if(!capId)
      throw upan::exception(XLOC, "Invalid Xtended CapID 0!");

    if(capId == 1)
    {
      if(!_legSupXCap)
        _legSupXCap = (LegSupXCap*)&xCapReg;
      else
        printf("\n Found more than 1 LEG SUP Extended capability entries - something wrong!!");
    }
    else if(capId == 2)
    {
      _supProtoXCaps.push_back((SupProtocolXCap*)&xCapReg);
    }
    else
    {
      printf("\n Unhandled Extended CapID: %d", capId);
    }
    unsigned nextOffset = (xCapReg >> 6) & 0x3FC;
    if(!nextOffset)
      break;
    base += nextOffset;
  }
  for(auto i : _supProtoXCaps)
    i->Print();
}

void XHCIController::PerformBiosToOSHandoff()
{
  if(!_legSupXCap)
  {
    printf("\n LEG SUP Extended capability is not supported - cannot perform BIOS to OS Handoff");
    return;
  }
  printf("\n Trying to perform complete handoff of XHCI Controller from BIOS to OS");
  _legSupXCap->BiosToOSHandoff();
}

USB_PROTOCOL XHCIController::PortProtocol(unsigned portId) const
{
  for(auto i : _supProtoXCaps)
  {
    if(i->HasPort(portId))
      return i->Protocol();
  }
  return USB_PROTOCOL::UNKNOWN;
}

const char* XHCIController::PortProtocolName(USB_PROTOCOL protocol) const
{
  switch(protocol)
  {
    case USB_PROTOCOL::USB3: return "usb3";
    case USB_PROTOCOL::USB2: return "usb2";
    case USB_PROTOCOL::USB1: return "usb1";
    default: return "unknown";
  }
}

const char* XHCIController::PortSpeedName(DEVICE_SPEED speed) const
{
  switch(speed)
  {
    case DEVICE_SPEED::FULL_SPEED: return "full speed";
    case DEVICE_SPEED::LOW_SPEED: return "low speed";
    case DEVICE_SPEED::HIGH_SPEED: return "high speed";
    default: return "undefined";
  }
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
    auto protocol = PortProtocol(i+1);
		printf("\n Setup Port: %d [%s] -> ", i, PortProtocolName(protocol));
    port.Print();
		if(_capReg->IsPPC())
      port.PowerOn();

    if(!port.IsPowerOn())
    {
      printf("\n Port %d is not Powered On!", i);
      continue;
    }

    if(port.IsDeviceConnected())
      printf("\n Device connected to Port %d", i);
    else
    {
      printf("\n No Device connected to Port %d", i);
      continue;
    }

    if(!port.IsEnabled())
    {
      try 
      {
        if(protocol == USB_PROTOCOL::USB2)
          port.Reset();
        else
          port.WarmReset();
        _eventManager->DebugPrint();
        if(!port.IsEnabled())
        {
          printf("\n Port still not enabled! ");
          port.Print();
          _opReg->Print();
          continue;
        }
      }
      catch(const upan::exception& ex)
      {
        printf("\n%s", ex.Error().c_str());
      }
    }

    printf("\n Connected Device Speed: %s", PortSpeedName(port.PortSpeedID()));
	}
}

CommandManager::CommandManager(XHCICapRegister& creg, XHCIOpRegister& oreg)
  : _pcs(true), _ring(nullptr), _capReg(creg), _opReg(oreg)
{
  _ring = new ((void*)DMM_AllocateAlignForKernel(sizeof(Ring), 64))Ring();

  LinkTRB link(_ring->_link, true);
  link.SetLinkAddr((uint64_t)&_ring->_cmd);
  link.SetToggleBit(true);

  uint64_t ringAddr = (unsigned)_ring - GLOBAL_DATA_SEGMENT_BASE;
  _opReg.SetCommandRingPointer(ringAddr);
}

EventManager::EventManager(XHCICapRegister& creg, XHCIOpRegister& oreg)
  : _ring(nullptr), _capReg(creg), _opReg(oreg)
{
  _ring = new ((void*)DMM_AllocateAlignForKernel(sizeof(Ring), 64))Ring();

  LinkTRB link(_ring->_link, true);
  link.SetLinkAddr((uint64_t)&_ring->_events[0]);
  link.SetToggleBit(true);

  uint64_t ringAddr = (unsigned)_ring - GLOBAL_DATA_SEGMENT_BASE;

  unsigned eventRingRegBase = ((unsigned)&creg) + _capReg.RTSOffset() + 32 * _capReg.MaxIntrs();

  //TODO: For now, set only 1 Event Ring Segment
  unsigned& ERSTZ = *(unsigned*)(eventRingRegBase + 0x28);
  ERSTZ = (ERSTZ & 0xFFFF0000) | 0x1;

  uint64_t& ERSTBA = *(uint64_t*)(eventRingRegBase + 0x30);
  ERSTBA = (ERSTBA & (uint64_t)(0x3F)) | ringAddr;

  uint64_t& ERDP = *(uint64_t*)(eventRingRegBase + 0x38);
  ERDP = (uint64_t)ringAddr;
}

void EventManager::DebugPrint() const
{
  const TRB& eventTRB = _ring->_events[0];
  printf("\n DW1: %x, DW2: %x, DW3: %x, DW4: %x", eventTRB._b1, eventTRB._b2, eventTRB._b3, eventTRB._b4);
}
