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
#include <PortCom.h>
#include <AsmUtil.h>
#include <PIC.h>
#include <DMM.h>
#include <PIC.h>
#include <MemUtil.h>
#include <Display.h>
#include <DMM.h>
#include <StringUtil.h>

#include <USBStructures.h>
#include <UHCIStructures.h>
#include <USBController.h>
#include <UHCIController.h>
#include <USBDataHandler.h>
#include <UHCIDevice.h>
#include <ProcessManager.h>
#include <stdio.h>

static const IRQ* UHCI_USB_IRQ ;

static void UHCIController_IRQHandler()
{
	unsigned GPRStack[NO_OF_GPR] ;
	AsmUtil_STORE_GPR(GPRStack) ;
	AsmUtil_SET_KERNEL_DATA_SEGMENTS

	KC::MDisplay().Message("\n USB IRQ \n", ' ') ;
	//ProcessManager_SignalInterruptOccured(HD_PRIMARY_IRQ) ;

	IrqManager::Instance().SendEOI(*UHCI_USB_IRQ) ;
	
	AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
	AsmUtil_RESTORE_GPR(GPRStack) ;
	
	asm("leave") ;
	asm("IRET") ;
}

UHCIController::UHCIController(PCIEntry* pPCIEntry, unsigned uiIOBase, unsigned uiIOSize) 
  : _pPCIEntry(pPCIEntry), _uiIOBase(uiIOBase & PCI_ADDRESS_IO_MASK), _uiIOSize(uiIOSize),
    _frameQueue(PAGE_TABLE_ENTRIES), _pFrameList(nullptr), _pLocalFrameList(nullptr)
{
  printf("\n Bus: %d, Device: %d, Function: %d", _pPCIEntry->uiBusNumber, _pPCIEntry->uiDeviceNumber, _pPCIEntry->uiFunction) ;

  printf("\n UHCI IO Addr = %x", _uiIOBase) ;
	printf("\n UHCI IO Size = ", _uiIOSize) ;

  /* disable legacy emulation */
  _pPCIEntry->WritePCIConfig(USB_LEGSUP, 2, 0) ;
  //_pPCIEntry->WritePCIConfig(USB_LEGSUP, 2, 0x100) ;
  unsigned short usCommand ;
  _pPCIEntry->ReadPCIConfig(USB_LEGSUP, 2, &usCommand) ;
  printf("\n USB LEGSUP: %x", usCommand) ;
//
  _pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand) ;
  printf("\n PCI COMMAND: %x", usCommand) ;
    //ProcessManager::Instance().Sleep(5000) ;
}

bool UHCIController::Probe()
{
	int iIRQ = _pPCIEntry->BusEntity.NonBridge.bInterruptLine ;
  printf("\n UHCI IRQ = %d", iIRQ) ;

  //TODO: Support multiple devices with same IRQ
	UHCI_USB_IRQ = IrqManager::Instance().RegisterIRQ(iIRQ, (unsigned)&UHCIController_IRQHandler) ;
	if(!UHCI_USB_IRQ)
  {
    printf("\n UHCI device with IRQ %d already activated", iIRQ);
		return false;
  }
	
	// As per UCHI Spec, there should be minimum 2 Ports present
	// 8th Bit from LSB is always set and that is used as a termination condition
	// of the Port iteration loop
	
	int iNumPorts ;
	int iPossibleNumPorts = ( _uiIOSize - PORTSC_REG ) / 2 ;

	for(iNumPorts = 0; iNumPorts < iPossibleNumPorts; iNumPorts++)
	{
		unsigned uiPortAddr = _uiIOBase + PORTSC_REG + iNumPorts * 2 ;

		unsigned short usPortStatus = PortCom_ReceiveWord(uiPortAddr) ;

		printf("\n USB Port: %d, Addr: %x, Status: %x", iNumPorts, uiPortAddr, usPortStatus) ;

		if(!(usPortStatus & 0x0080))
			break ;
	}

  printf("\n Detected Ports: %d", iNumPorts) ;

	if(iNumPorts < 2 || iNumPorts > 8)
		iNumPorts = 2 ;

	//Reset Hub
	/* Global reset for 50ms */
	PortCom_SendWord(_uiIOBase + USBCMD_REG, USBCMD_GRESET) ;
	ProcessManager::Instance().Sleep(100) ;
	PortCom_SendWord(_uiIOBase + USBCMD_REG, 0) ;
	ProcessManager::Instance().Sleep(100) ;

	//Start Hub
	/* Reset the HC - this will force us to get a new notification of any
	 * already connected ports due to the virtual disconnect that it
	 * implies.
	 */
	unsigned uiLimit = 1000;
	PortCom_SendWord(_uiIOBase + USBCMD_REG, USBCMD_HCRESET) ;
	ProcessManager::Instance().Sleep(50) ;
	
	while(PortCom_ReceiveWord(_uiIOBase + USBCMD_REG) & USBCMD_HCRESET)
	{
		// Spec says we should wait for 10ms before HCRESET is set to 0
		if(uiLimit == 1)
			ProcessManager::Instance().Sleep(10) ;

		if(! (--uiLimit) )
		{
      printf("\n USBCMD_HCRESET TimeOut!") ;
			break;
		}
	}

	/* Start at frame 0 */
	PortCom_SendWord(_uiIOBase + FRNUM_REG, 0) ;

	CreateFrameList() ;

	/* Turn on all interrupts */
	PortCom_SendWord(_uiIOBase + USBINTR_REG, USBINTR_SHORT_PACKET | USBINTR_IOC | USBINTR_RESUME | USBINTR_TIMEOUT_CRC) ;

	/* Run and mark it configured with a 64-byte max packet */
	PortCom_SendWord(_uiIOBase + USBCMD_REG, (USBCMD_RS | USBCMD_CF | USBCMD_MAXP)) ;

	ProcessManager::Instance().Sleep(200) ;

	// Check Status
	// For the time just checking for zero
	unsigned short usWord = 0;
	if((usWord = PortCom_ReceiveWord(_uiIOBase + USBSTS_REG)) != 0x0)
	{
		KC::MDisplay().Address("\n Error. UHCI Host Controller Status = ", usWord) ;
		usWord = PortCom_ReceiveWord(_uiIOBase + USBCMD_REG) ;
		printf("\n CMD: %x", usWord) ;

		usWord = PortCom_ReceiveWord(_uiIOBase + FRNUM_REG) ;
		printf("\n FRNUM: %x", usWord) ;

		unsigned uiWord = PortCom_ReceiveDoubleWord(_uiIOBase + FLBASEADD_REG) ;
		printf("\n FR BASE: %x", uiWord) ;

		usWord = PortCom_ReceiveWord(_uiIOBase + USBINTR_REG) ;
		printf("\n INT: %x", usWord) ;

		return false ;
	}

	// Consistency Check
	// Assert for Default value of SOF register
	usWord = 0;
	if((usWord = PortCom_ReceiveByte(_uiIOBase + SOF_REG)) != 0x40)
	{
    printf("\n Default of SOF register is not 0x40. --> %x", usWord) ;
		return false ;
	}
	// End of Start Hub

	/* Enable PIRQ - Legacy Mouse and Keyboard Support for 8042 */
	_pPCIEntry->WritePCIConfig(USB_LEGSUP, 2, USB_LEGSUP_DEFAULT) ;

	printf("\n USB UHCI at I/O: %x, IRQ: %d, Detected Ports: %d ", _uiIOBase, iIRQ, iNumPorts) ;

	ProcessManager::Instance().Sleep(100) ;

	for(int i = 0; i < iNumPorts; i++)
	{
		unsigned uiPortAddr = _uiIOBase + PORTSC_REG + i * 2 ;
		unsigned short status = PortCom_ReceiveWord(uiPortAddr) ;

		printf("\n Current Port %d Status %x", i + 1, status) ;
		if(status & 0x1)
		{
			if((status & 0x4) == 0)
			{
				status |= 0x200 ;
				printf("\n Setting Port: %d status to %x", i + 1, status) ;
				PortCom_SendWord(uiPortAddr, status) ;
				
				ProcessManager::Instance().Sleep(100) ;
				
				status = PortCom_ReceiveWord(uiPortAddr) ;
				printf("\n New Port Status = %x", status) ;
				
				status &= ~(0x200) ;
				PortCom_SendWord(uiPortAddr, status) ;

				ProcessManager::Instance().Sleep(100) ;
			
				status = PortCom_ReceiveWord(uiPortAddr) ;
				status |= 0x4 ;
				PortCom_SendWord(uiPortAddr, status) ;

				ProcessManager::Instance().Sleep(100) ;

				status = PortCom_ReceiveWord(uiPortAddr) ;
				status |= 0xA ;
				PortCom_SendWord(uiPortAddr, status) ;

				ProcessManager::Instance().Sleep(100) ;

				status = PortCom_ReceiveWord(uiPortAddr) ;
				printf("\n After Reset Port Status = %x", status) ;
			}
		}

		if((status & 0x5) == 0x5)
    {
      USBDriver* pDriver = USBController::Instance().FindDriver(new UHCIDevice(*this, uiPortAddr));

      if(pDriver)
        printf("\n'%s' driver found for the USB Device\n", pDriver->Name().c_str()) ;
      else
        printf("\nNo Driver found for this USB device\n") ;
    }
	}

  _frameQueue.clear();
  KernelUtil::ScheduleTimedTask("UHCIFrmCleaner", 100, *this);

	return true;
}

bool UHCIController::GetNextFrameToClean(unsigned& uiFrameNumber)
{
  ResourceMutexGuard rg(RESOURCE_UHCI_FRM_BUF);

  if(_frameQueue.empty())
    return false;

  uiFrameNumber = _frameQueue.front();
  _frameQueue.pop_front();

	return true;
}

bool UHCIController::CleanFrame(unsigned uiFrameNumber)
{
  MutexGuard g(_m);

  if(_frameQueue.full())
		return false;
  else
    _frameQueue.push_back(uiFrameNumber);

	return false;
}

bool UHCIController::TimerTrigger()
{
	unsigned uiFrameNumber ;
	while(GetNextFrameToClean(uiFrameNumber))
	{
		_pFrameList[ uiFrameNumber ] |= TD_LINK_TERMINATE ;

		ReleaseFrameResource(uiFrameNumber) ;

		_pFrameList[ uiFrameNumber ] = 1 ;
	}
  return true;
}

void UHCIController::CreateFrameList()
{
	unsigned uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();

	_pFrameList = (unsigned*)(uiFreePageNo * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE) ;

	for(unsigned i = 0; i < 1024; i++)
		_pFrameList[i] = 1;

	_pLocalFrameList = new upan::list<unsigned>[1024];

	PortCom_SendDoubleWord(_uiIOBase + FLBASEADD_REG, (unsigned)_pFrameList + GLOBAL_DATA_SEGMENT_BASE) ;
}

unsigned UHCIController::GetFrameListEntry(unsigned uiFrameNumber)
{
	return _pFrameList[ uiFrameNumber ] ;
}

void UHCIController::SetFrameListEntry(unsigned uiFrameNumber, unsigned uiValue, bool bCleanBuffer, bool bCleanDescs)
{
	BuildFrameEntryForDeAlloc(uiFrameNumber, uiValue, bCleanBuffer, bCleanDescs) ;
	_pFrameList[ uiFrameNumber ] = uiValue ;
}

unsigned UHCIController::GetNextFreeFrame()
{
  ResourceMutexGuard rg(RESOURCE_UHCI_FRM_BUF);

	for(int i = 0; i < 1024; i++)
	{
		if(_pFrameList[i] == 1)
		{
			_pFrameList[i] = 0 ;
      return i;
		}	
	}
  throw upan::exception(XLOC, "UHCI - no free frames left");	
}

void UHCIController::BuildFrameEntryForDeAlloc(unsigned uiFrameNumber, unsigned uiDescAddress, bool bCleanBuffer, bool bCleanDescs)
{
	if(uiDescAddress == NULL)
		return ;

  auto& frameList = _pLocalFrameList[uiFrameNumber];

	if(uiDescAddress & TD_LINK_QH_POINTER)
	{
		uiDescAddress = UHCI_DESC_ADDR(uiDescAddress) - GLOBAL_DATA_SEGMENT_BASE ;
		
		UHCIQueueHead* pQH = (UHCIQueueHead*)uiDescAddress ;
		unsigned uiElementLinkPointer = pQH->ElementLinkPointer();
		unsigned uiHeadLinkPointer = pQH->HeadLinkPointer();
		
		if(!(uiElementLinkPointer & TD_LINK_TERMINATE))
		{
			BuildFrameEntryForDeAlloc(uiFrameNumber, uiElementLinkPointer, bCleanBuffer, bCleanDescs) ;
		}

		if(!(uiHeadLinkPointer & TD_LINK_TERMINATE))
		{
			BuildFrameEntryForDeAlloc(uiFrameNumber, uiHeadLinkPointer, bCleanBuffer, bCleanDescs) ;
		}

    frameList.push_back(uiDescAddress);
	}
	else
	{
		uiDescAddress = UHCI_DESC_ADDR(uiDescAddress) - GLOBAL_DATA_SEGMENT_BASE ;

		UHCITransferDesc* pTD = (UHCITransferDesc*)uiDescAddress ;
		unsigned uiLinkPointer = pTD->LinkPointer();

		if(!(uiLinkPointer & TD_LINK_TERMINATE))
		{
			BuildFrameEntryForDeAlloc(uiFrameNumber, uiLinkPointer, bCleanBuffer, bCleanDescs) ;
		}

		if(bCleanBuffer)
		{
			unsigned uiBufferPointer = pTD->BufferPointer();
			if(uiBufferPointer != NULL)
			{
				uiBufferPointer -= GLOBAL_DATA_SEGMENT_BASE ;

				if(uiBufferPointer != NULL)
          frameList.push_back(uiBufferPointer);
			}
		}

		if(bCleanDescs)
      frameList.push_back(uiDescAddress);
	}
}

void UHCIController::ReleaseFrameResource(unsigned uiFrameNumber)
{
  auto& frameList = _pLocalFrameList[uiFrameNumber];

  for(auto i : frameList)
  {
    if(i != NULL)
		  DMM_DeAllocateForKernel(i);
  }
  frameList.clear();
}
