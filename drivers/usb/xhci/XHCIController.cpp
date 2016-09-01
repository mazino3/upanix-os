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
#include <uniq_ptr.h>
#include <KBDriver.h>
#include <XHCIController.h>
#include <XHCIContext.h>
#include <XHCIManager.h>
#include <USBDataHandler.h>

unsigned XHCIController::_memMapBaseAddress = XHCI_MMIO_BASE_ADDR;

XHCIController::XHCIController(PCIEntry* pPCIEntry)
  : _pPCIEntry(pPCIEntry), _capReg(nullptr), _opReg(nullptr),
    _legSupXCap(nullptr), _doorBellRegs(nullptr)
{
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
  const unsigned uiMappedIOAddr = KERNEL_VIRTUAL_ADDRESS(_memMapBaseAddress + (uiIOAddr % PAGE_SIZE));
  printf("\n Total pages to Map: %d", pagesToMap);
  for(unsigned i = 0; i < pagesToMap; ++i)
  {
  	unsigned uiPDEIndex = ((_memMapBaseAddress >> 22) & 0x3FF) ;
	  unsigned uiPTEIndex = ((_memMapBaseAddress >> 12) & 0x3FF) ;
	  unsigned uiPTEAddress = (((unsigned*)(KERNEL_VIRTUAL_ADDRESS(uiPDEAddress)))[uiPDEIndex]) & 0xFFFFF000 ;
    // This page is a Read Only area for user process. 0x5 => 101 => User Domain, Read Only, Present Bit
    ((unsigned*)(KERNEL_VIRTUAL_ADDRESS(uiPTEAddress)))[uiPTEIndex] = (uiIOAddr & 0xFFFFF000) | 0x5 ;
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
  _deviceContextAddrArray = (uint64_t*)KERNEL_VIRTUAL_ADDRESS(deviceContextTable);
  memset((void*)_deviceContextAddrArray, 0, PAGE_SIZE);
  _opReg->SetDCBaap(deviceContextTable);

  //Door Bell array
  _doorBellRegs = (unsigned*)(uiMappedIOAddr + _capReg->DoorBellOffset());

  InitInterruptHandler();

  //Setup Event Ring
  _eventManager = new EventManager(*this, *_capReg, *_opReg);

  //Setup Command Ring
  _cmdManager = new CommandManager(*_capReg, *_opReg, *_eventManager);
}

void XHCIController::Start()
{
  if(_opReg->IsHCRunning())
    return;
  _opReg->DisableHCInterrupt();
  _opReg->Run();
  ProcessManager::Instance().Sleep(100);
  if(!_opReg->IsHCRunning() || _opReg->IsHCHalted())
    throw upan::exception(XLOC, "Failed to start XHCI HC");
  _opReg->Print();
  _opReg->EnableHCInterrupt();
}

//Should be called from IRQ handler - so no need for any synchronization constructs
void XHCIController::NotifyEvent()
{
  if(!_opReg->StatusChanged())
    return;

  try
  {
    if(_opReg->IsHCHalted())
    {
      printf("\n XHCI host controller halted!!");
    }
    if(_opReg->IsHSError())
    {
      printf("\n XHCI host system error!!");
    }
    if(_opReg->IsAnyEventPending())
    {
      _eventManager->NotifyEvents();
    }
    if(_opReg->Saving())
    {
      printf("\n Saving XHCI internal state");
    }
    if(_opReg->Restoring())
    {
      printf("\n Restoring XHCI internal state");
    }
    if(_opReg->IsSRError())
    {
      printf("\n XHCI Save/Restore error!!");
    }
    if(_opReg->IsHCNotReady())
    {
      printf("\n XHCI host controller is not ready!!");
    }
    if(_opReg->IsHCError())
    {
      printf("\n XHCI host controller error!!");
    }
  }
  catch(const upan::exception& e)
  {
    printf("\n Exception while handling XHCI event - %s", e.Error().c_str());
  }

  _opReg->Clear();
}

void XHCIController::InitInterruptHandler()
{
  //Can only support MSI capable XHCI
  if(!_pPCIEntry->SetupMsiInterrupt(XHCI_IRQ_NO))
    throw upan::exception(XLOC, "XHCI is not capable of MSI interrupts!");
  _pPCIEntry->SwitchToMsi();
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

SupProtocolXCap* XHCIController::GetSupportedProtocol(unsigned portId) const
{
  for(auto i : _supProtoXCaps)
  {
    if(i->HasPort(portId))
      return i;
  }
  return nullptr;
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

void XHCIController::Probe()
{
  Start();
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
    auto supProtocol = GetSupportedProtocol(i+1);
    if(!supProtocol)
    {
      printf("\n There is Extended Supported Protocol Cap for Port: %d !", i);
      continue;
    }
    auto protocol = supProtocol->Protocol();
		printf("\n Setup Port: %d [%s] -> ", i, PortProtocolName(protocol));
    port.Print();
		if(_capReg->IsPPC())
      port.PowerOn();

    if(!port.IsPowerOn())
    {
      printf("\n Port %d is not Powered On!", i);
      continue;
    }

    if(!port.IsDeviceConnected())
    {
      printf("\n No Device connected to Port %d", i);
      continue;
    }

    if(!port.IsEnabled())
    {
      try 
      {
        if(protocol == USB_PROTOCOL::USB2)
        {
          port.Reset();
          _eventManager->DebugPrint();
        }
        else
          port.WarmReset();
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

    port.Print();
    PortSpeed ps = supProtocol->PortSpeedInfo(port.PortSpeedID());
    printf("\n %s [%u %s] device connected to port %d", PortProtocolName(protocol), ps.Mantissa(), ps.BitRateS(), i);
    InitializeDevice(port, i+1, supProtocol->SlotType());
	}
}

void XHCIController::RingDoorBell(unsigned index, unsigned value)
{
  _doorBellRegs[index] = value;
//	Atomic::Swap(_doorBellRegs[index], value);
}

void XHCIController::InitializeDevice(XHCIPortRegister& port, unsigned portId, unsigned slotType)
{
  unsigned slotID = EnableSlot(slotType);
  if(!slotID)
    throw upan::exception(XLOC, "Failed to get SlotID");

  TransferRing* tRing = new TransferRing(64);
  uint32_t trRingPtr = KERNEL_REAL_ADDRESS(tRing->RingBase());

  InputContext* inputContext = new InputContext(_capReg->IsContextSize64(), port, portId, 0, trRingPtr);
  DeviceContext* devContext = new DeviceContext(_capReg->IsContextSize64());
  //set A0 and A1 -> Slot and EP0 are affected by command
  inputContext->Control().SetAddContextFlag(0x3);

  _deviceContextAddrArray[slotID] = (uint64_t)KERNEL_REAL_ADDRESS(&devContext->Slot());

//TODO: to deal with older devices - you may have to send 2 AddressDevice twice
//first, with block bit set and then with block bit cleared
//With first request, address should be set and slot stat is default (1)
//With second request, slot state should change to Addressed (2)
  AddressDevice((unsigned)&inputContext->Control(), slotID, false);

  if(devContext->EP0().EPState() != EndPointContext::Running)
    throw upan::exception(XLOC, "After AddressDevice, EndPoint0 is in %d state", devContext->EP0().EPState());

  if(devContext->Slot().SlotState() != SlotContext::Addressed)
    throw upan::exception(XLOC, "After AddressDevice, Slot is in %d state", devContext->Slot().SlotState());

  printf("\n USB Device Address: %x, PortMaxPkSz: %u, ControlEP MaxPktSz: %u", 
    devContext->Slot().DevAddress(),
    port.MaxPacketSize(),
    devContext->EP0().MaxPacketSize());

  //** Get device descriptor **
  //the buffer has to be on kernel heap - a mem area that is 1-1 mapped b/w virtual (page) and physical
  //as it's used by XHCI controller to transfer data
  upan::uniq_ptr<USBStandardDeviceDesc> deviceDesc(new USBStandardDeviceDesc());
  uint32_t len = sizeof(USBStandardDeviceDesc);
  //Setup stage
  tRing->AddSetupStageTRB(0x80, 6, 0x100, 0, len, TransferType::IN_DATA_STAGE);
  //Data stage
  tRing->AddDataStageTRB(KERNEL_REAL_ADDRESS(deviceDesc.get()), len, DataDirection::IN, port.MaxPacketSize());
  //Status Stage
  uint32_t trdId = tRing->AddStatusStageTRB();
  RegisterForEventResult(trdId);
  //Doorbell
  RingDoorBell(slotID, 1);
  //Wait for Interrupt
  EventTRB result;
  WaitForTransferCompletion(trdId, result);
  deviceDesc->DebugPrint();

//  inputContext->Control().SetAddContextFlag(0x1);
//  ConfigureEndPoint((unsigned)&inputContext->Control(), slotID);
}

unsigned XHCIController::EnableSlot(unsigned slotType)
{
  _cmdManager->EnableSlot(slotType);
  RegisterForEventResult(_cmdManager->CommandTRBAddress());
  RingDoorBell(0, 0);

  EventTRB result;
  WaitForCmdCompletion(result);
  return result.SlotID();
}

void XHCIController::AddressDevice(unsigned icptr, unsigned slotID, bool blockSetAddressReq)
{
  _cmdManager->AddressDevice(icptr, slotID, blockSetAddressReq);
  RegisterForEventResult(_cmdManager->CommandTRBAddress());
  RingDoorBell(0, 0);

  EventTRB result;
  WaitForCmdCompletion(result);
}

void XHCIController::ConfigureEndPoint(unsigned icptr, unsigned slotID)
{
  _cmdManager->ConfigureEndPoint(icptr, slotID);
  RegisterForEventResult(_cmdManager->CommandTRBAddress());
  RingDoorBell(0, 0);

  EventTRB result;
  WaitForCmdCompletion(result);
}

void XHCIController::WaitForCmdCompletion(EventTRB& result)
{
  WaitForEvent(_cmdManager->CommandTRBAddress(), result);
  if(result.Type() != 33)
    throw upan::exception(XLOC, "Got invalid Event TRB: %d", result.Type());

  if(!result.IsCommandSuccess())
    throw upan::exception(XLOC, "Command did not complete successfully");
}

void XHCIController::WaitForTransferCompletion(uint32_t trbId, EventTRB& result)
{
  WaitForEvent(trbId, result);
  if(result.Type() != 32)
    throw upan::exception(XLOC, "Got invalid Event TRB: %d", result.Type());

  if(!result.IsCommandSuccess())
    throw upan::exception(XLOC, "Transfer command did not complete successfully");

//  if(!result.IsEventDataTRB())
  //  throw upan::exception(XLOC, "Transfer event is not Event Data TRB");
}

void XHCIController::WaitForEvent(uint32_t trbId, EventTRB& result)
{
  if(XHCIManager::Instance().GetEventMode() == XHCIManager::Poll)
  {
    if(!_eventManager->WaitForEvent(result))
      throw upan::exception(XLOC, "Timedout while waiting for Command Completion");
  }
  else
  {
    ProcessManager::Instance().WaitForEvent();
    result = ConsumeEventResult(trbId).Result();
  }
}

void XHCIController::RegisterForEventResult(uint32_t trbId)
{
  //TODO: Find an efficient way w/o disabling interrupts
  IrqGuard g;
  _eventResults[trbId] = EventResult(ProcessManager::Instance().GetCurProcId());
}

XHCIController::EventResult XHCIController::ConsumeEventResult(uint32_t trbId)
{
  //TODO: Find an efficient way w/o disabling interrupts
  IrqGuard g;
  auto it = _eventResults.find(trbId);
  if(it == _eventResults.end())
    throw upan::exception(XLOC, "Can't find TRB ID: %x in EventResults", trbId);
  auto result = it->second;
  _eventResults.erase(it);
  return result;
}

//This function is called from XHCI IRQ handler, hence doesn't need any
//synchronization construct i.e. IrqGuard/ProcessSwitchLock/Mutex
void XHCIController::PublishEventResult(const EventTRB& result)
{
  auto it = _eventResults.find(result.TRBPointer());
  if(it == _eventResults.end())
  {
    printf("\n No entry found in EventResults for TRB Id: %x, Event Type: %d", (uint32_t)result.TRBPointer(), result.Type());
    return;
  }
  EventResult& r =  it->second;
  r.Result(result);
  ProcessManager::Instance().EventCompleted(r.Pid());
}

CommandManager::CommandManager(XHCICapRegister& creg, 
  XHCIOpRegister& oreg,
  EventManager& eventManager)
  : _pcs(true), _ring(nullptr), _capReg(creg), _opReg(oreg), _eventManager(eventManager)
{
  _ring = new ((void*)DMM_AllocateForKernel(sizeof(Ring), 64))Ring();
  uint64_t ringAddr = KERNEL_REAL_ADDRESS(_ring);

  _ring->_link.SetLinkAddr(ringAddr);
  _ring->_link.SetToggleBit(true);

  _opReg.SetCommandRingPointer(ringAddr);
}

void CommandManager::Apply()
{
  _ring->_cmd.SetCycleBit(_pcs);
  _ring->_link.SetCycleBit(_pcs);
  _pcs = !_pcs;
}

void CommandManager::DebugPrint()
{
  printf("\n Command Ring - ");
  _ring->_cmd.Print();
  _ring->_link.Print();
}

void CommandManager::EnableSlot(unsigned slotType)
{
  _ring->_cmd = EnableSlotTRB(slotType);
  Apply();
}

void CommandManager::DisableSlot(unsigned slotID)
{
  _ring->_cmd = DisableSlotTRB(slotID);
  Apply();
}

void CommandManager::AddressDevice(unsigned icptr, unsigned slotID, bool blockSetAddressReq)
{
  _ring->_cmd = AddressDeviceTRB(icptr, slotID, blockSetAddressReq);
  Apply();
}

void CommandManager::ConfigureEndPoint(unsigned icptr, unsigned slotID)
{
  _ring->_cmd = ConfigureEndPointTRB(icptr, slotID);
  Apply();
}

EventManager::InterrupterRegister::InterrupterRegister() 
{
  _iman = 0; //Interrupter is disabled by default
  _imod = 4000; //15:0 = 4000 --> 1ms, 31:16 = 0 --> initial count
  const int ERST_SIZE = 1;
  _erstSize = (_erstSize & 0xFFFF0000) | ERST_SIZE;

  ERSTEntry* erst = new ((void*)DMM_AllocateForKernel(sizeof(ERSTEntry) * ERST_SIZE, 64))ERSTEntry[ERST_SIZE];
  _erstBA = (uint64_t)KERNEL_REAL_ADDRESS(erst);

  _erdqPtr = erst[0]._ersAddr;
}

void EventManager::InterrupterRegister::DebugPrint()
{
  printf("\n IMAN: %x, IMON: %x, DQPTR: %x", _iman, _imod, (unsigned)DQPtr());
//  _erdqPtr = _erdqPtr | 0x8;
//  _iman = _iman | 0x1;
  for(int i = 0; i < 16; ++i)
    ERSegment(0)[i].Print();
}

const int EventManager::ERST_SIZE = 256;
EventManager::ERSTEntry::ERSTEntry() : _size(ERST_SIZE)
{
  unsigned eventSegmentTable = MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE;
  EventTRB* events = new ((void*)KERNEL_VIRTUAL_ADDRESS(eventSegmentTable))EventTRB[_size];
  _ersAddr = (uint64_t)KERNEL_REAL_ADDRESS(events);
}

EventManager::EventManager(XHCIController& controller, XHCICapRegister& creg, XHCIOpRegister& oreg)
  : _eventCycleBit(true), _capReg(creg), _opReg(oreg), _controller(controller)
{
  _iregs = (InterrupterRegister*)((unsigned)&_capReg + _capReg.RTSOffset() + 0x20);
  for(unsigned i = 0; i < _capReg.MaxIntrs(); ++i)
    new ((void*)&_iregs[i])InterrupterRegister();
  _iregs[0].EnableInterrupt();
}

void EventManager::DebugPrint() const
{
  printf("\n Event Ring - ");
  _iregs[0].DebugPrint();
}

bool EventManager::WaitForEvent(EventTRB& result)
{
  int timeout = 2000;//2 seconds
  while(timeout > 10)
  {
    if(_iregs[0].EventCycleBitSet() == _eventCycleBit)
    {
      if(_iregs[0].IsLastDQPtr())
        _eventCycleBit = !_eventCycleBit;
      result = *_iregs[0].DQEvent();
      _iregs[0].IncrementDQPtr();
      return true;
    }
    ProcessManager::Instance().Sleep(10);
    timeout -= 10;
  }
  return false;
}

void EventManager::NotifyEvents()
{
  unsigned count = 0;
  while(_iregs[0].EventCycleBitSet() == _eventCycleBit)
  {
    if(_iregs[0].IsLastDQPtr())
      _eventCycleBit = !_eventCycleBit;

    _controller.PublishEventResult(*_iregs[0].DQEvent());

    _iregs[0].IncrementDQPtr();

    ++count;
    if(count == ERST_SIZE)
      break;
  }
}
