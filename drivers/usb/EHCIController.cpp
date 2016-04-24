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
#include <USBMassBulkStorageDisk.h>
#include <PCIBusHandler.h>
#include <Display.h>
#include <DMM.h>
#include <MemManager.h>
#include <MemUtil.h>
#include <PortCom.h>
#include <stdio.h>
#include <cstring.h>
#include <StringUtil.h>

#include <USBStructures.h>
#include <USBDataHandler.h>
#include <EHCIDataHandler.h>
#include <EHCIController.h>
#include <EHCIDevice.h>

EHCIController::EHCIController(PCIEntry* pPCIEntry, int iMemMapIndex) 
  : _pPCIEntry(pPCIEntry), _pCapRegs(nullptr), _pOpRegs(nullptr), _pAsyncReclaimQueueHead(nullptr)
{
//	if(!pPCIEntry->BusEntity.NonBridge.bInterruptLine)
  //  throw upan::exception(XLOC, "EHCI device with no IRQ. Check BIOS/PCI settings!");

	unsigned uiIOAddr = pPCIEntry->BusEntity.NonBridge.uiBaseAddress0;
	printf("\n PCI BaseAddr: %x", uiIOAddr);

	uiIOAddr = uiIOAddr & PCI_ADDRESS_MEMORY_32_MASK;
	unsigned uiIOSize = pPCIEntry->GetPCIMemSize(0);
	printf(", Raw MMIO BaseAddr: %x, IOSize: %d", uiIOAddr, uiIOSize);

	if(uiIOSize > PAGE_SIZE)
    throw upan::exception(XLOC, "EHCI IO Size greater then 1 Page (4096b) not supported currently !");

	if((uiIOAddr % PAGE_SIZE) + uiIOSize > PAGE_SIZE)
    throw upan::exception(XLOC, "EHCI MMIO area is spanning across PAGE boundary. This is not supported in Upanix!!");

	// Currently an IO Size of not more than a PAGE_SIZE is supported
	// And also the IO Addr is expected not to span a PAGE Boundary.
	// Now, the mapping EHCI_MMIO_BASE_ADDR is choosen in such way that
	// EHCI_MMIO_BASE_ADDR and EHCI_MMIO_BASE_ADDR + 32 * PAGE_SIZE fall
	// within the same PTE Entry
	// Further, mapping is necessary because the IOAddr can be any virtual address
	// within 4 GB space potentially being an address outside the RAM size
	// i.e, PDE/PTE limit
	unsigned uiPDEAddress = MEM_PDBR ;
	unsigned uiMapAddress = EHCI_MMIO_BASE_ADDR + iMemMapIndex * PAGE_SIZE;
	unsigned uiPDEIndex = ((uiMapAddress >> 22) & 0x3FF) ;
	unsigned uiPTEIndex = ((uiMapAddress >> 12) & 0x3FF) ;

	unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000 ;
	// This page is a Read Only area for user process. 0x5 => 101 => User Domain, Read Only, Present Bit
	((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = (uiIOAddr & 0xFFFFF000) | 0x5 ;
	Mem_FlushTLB();
	
	if(MemManager::Instance().MarkPageAsAllocated(uiIOAddr / PAGE_SIZE) != Success)
	{
	}

	uiMapAddress = uiMapAddress + (uiIOAddr % PAGE_SIZE) - GLOBAL_DATA_SEGMENT_BASE ;
	_pCapRegs = (EHCICapRegisters*)uiMapAddress;
	byte bCapLen = _pCapRegs->bCapLength ;
	_pOpRegs = (EHCIOpRegisters*)(uiMapAddress + bCapLen) ;

	unsigned uiHCSParams = _pCapRegs->uiHCSParams ;
	unsigned uiHCCParams = _pCapRegs->uiHCCParams ;
	unsigned uiConfigFlag = _pOpRegs->uiConfigFlag ;

	printf("\n HCSPARAMS: %x, HCCPARAMS: %x, CAPLen: %x, CFBit: %d", uiHCSParams, uiHCCParams, bCapLen, uiConfigFlag) ;
	printf(", Bus: %d, Dev: %d, Func: %d", pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction) ;
	/* Enable busmaster */
	unsigned short usCommand ;
	pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
	printf("\n CurVal of PCI_COMMAND: %x", usCommand) ;
	pPCIEntry->WritePCIConfig(PCI_COMMAND, 2, usCommand | PCI_COMMAND_IO | PCI_COMMAND_MASTER) ;
	printf(" -> After Bus Master Enable, PCI_COMMAND: %x", usCommand) ;
}

void EHCIController::DisplayStats()
{
	printf("\n Cmd: %x, Status: %x, Interrupt: %x", _pOpRegs->uiUSBCommand, _pOpRegs->uiUSBStatus, _pOpRegs->uiUSBInterrupt);
}

bool EHCIController::PollWait(unsigned* pValue, int iBitPos, unsigned value)
{
	value &= 1;
	if(iBitPos > 31 || iBitPos < 0)
		return false ;

	int iMaxLimit = 10000 ; // 10 Sec
	unsigned uiSleepTime = 10 ; // 10 ms

	while(iMaxLimit > 10)
	{
		if( (((*pValue) >> iBitPos) & 1) == value )
			return true ;

		ProcessManager::Instance().Sleep(uiSleepTime) ;
		iMaxLimit -= uiSleepTime ;
	}

	return false ;
}

void EHCIController::AddAsyncQueueHead(EHCIQueueHead* pQH)
{
	pQH->uiHeadHorizontalLink = _pAsyncReclaimQueueHead->uiHeadHorizontalLink ;
	_pAsyncReclaimQueueHead->uiHeadHorizontalLink = KERNEL_REAL_ADDRESS(pQH) | 0x2 ;
}

byte EHCIController::SetConfigFlag(bool bSet)
{
	unsigned uiCompareValue = bSet ? 0 : 1 ;
	unsigned uiConfigFlag = _pOpRegs->uiConfigFlag ;

	if((uiConfigFlag & 0x1) == uiCompareValue)
	{
		if(bSet)
			printf("\n Routing ports to main EHCI controller. Turning on CF Bit") ;
		else
			printf("\n Routing ports to companion host controller. Turning off CF Bit") ;

		if(bSet)
			uiConfigFlag |= 1 ;
		else
			uiConfigFlag &= ~(1) ;

		_pOpRegs->uiConfigFlag = uiConfigFlag ;

		ProcessManager::Instance().Sleep(100) ;
		if((_pOpRegs->uiConfigFlag & 0x1) == uiCompareValue)
		{
			printf("\n Failed to Set Config Flag to: %d:", bSet ? 1 : 0) ;
			return EHCIController_FAILURE ;
		}
	}
	else
	{
		printf("\n Config Bit is already set to: %d", bSet ? 1 : 0) ;
	}

	return EHCIController_SUCCESS ;
}

void EHCIController::SetupInterrupts()
{
	_pOpRegs->uiUSBInterrupt = _pOpRegs->uiUSBInterrupt	| INTR_ASYNC_ADVANCE 
																		| INTR_HOST_SYS_ERR 
																		| INTR_PORT_CHG 
																		| INTR_USB_ERR 
																		| INTR_USB ;
	_pOpRegs->uiUSBInterrupt = 0 ;
}

byte EHCIController::SetupPeriodicFrameList()
{
	unsigned uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();

	unsigned* pFrameList = (unsigned*)(uiFreePageNo * PAGE_SIZE - GLOBAL_DATA_SEGMENT_BASE) ;
	
	int i ;
	for(i = 0; i < 1024; i++)
		pFrameList[ i ] = 0x1 ;

	_pOpRegs->uiPeriodicListBase = (unsigned)pFrameList + GLOBAL_DATA_SEGMENT_BASE ;

	return EHCIController_SUCCESS;
}

byte EHCIController::SetupAsyncList()
{
	unsigned uiQHAddress = DMM_AllocateAlignForKernel(sizeof(EHCIQueueHead), 32);
	memset((void*)(uiQHAddress), 0, sizeof(EHCIQueueHead));
	_pOpRegs->uiAsyncListBase = KERNEL_REAL_ADDRESS(uiQHAddress);

	EHCIQueueHead* pQHH = (EHCIQueueHead*)uiQHAddress;

	pQHH->uiCurrrentpTDPointer = 0;
	pQHH->uiNextpTDPointer = 1;
	pQHH->uiAltpTDPointer_NAKCnt = 1;
	
	pQHH->uiEndPointCap_Part1 = (1 << 15);
	pQHH->uipQHToken = (1 << 6);

	pQHH->uiHeadHorizontalLink = KERNEL_REAL_ADDRESS(uiQHAddress) | 0x2;

	_pAsyncReclaimQueueHead = pQHH;

	return EHCIController_SUCCESS;
}

void EHCIController::SetSchedEnable(unsigned uiScheduleType, bool bEnable)
{
	if(bEnable)
		_pOpRegs->uiUSBCommand |= uiScheduleType ;
	else
		_pOpRegs->uiUSBCommand &= ~(uiScheduleType) ;
}

void EHCIController::SetFrameListSize()
{
	if((_pCapRegs->uiHCCParams & 0x20) != 0)
		_pOpRegs->uiUSBCommand &= ~(0xC) ;
}

void EHCIController::Start()
{
	_pOpRegs->uiUSBCommand |= 1 ;
}

void EHCIController::Stop()
{
	_pOpRegs->uiUSBCommand &= 0xFFFFFFFE ;
}

bool EHCIController::CheckHCActive()
{
	return (_pOpRegs->uiUSBCommand & 1) ? true : false ;
} 

bool EHCIController::WaitCheckAsyncScheduleStatus(bool bValue)
{
	bool bStatus = PollWait(&(_pOpRegs->uiUSBCommand), 5, bValue) ;

	if(bStatus)
		bStatus = PollWait(&(_pOpRegs->uiUSBStatus), 15, bValue) ;

	return bStatus ;
}

bool EHCIController::StartAsyncSchedule()
{
	SetSchedEnable(ASYNC_SCHEDULE_ENABLE, true) ;

	if(!WaitCheckAsyncScheduleStatus(true))
	{
		printf("\n Failed to Start Async Schedule") ;
		DisplayStats() ;
		return false ;
	}

	if(!CheckHCActive())
	{
		printf("\n Controller has stopped !!!") ;
		DisplayStats() ;
		return false ;
	}

	return true ;
}

bool EHCIController::StopAsyncSchedule()
{
	SetSchedEnable(ASYNC_SCHEDULE_ENABLE, false) ;

	if(!WaitCheckAsyncScheduleStatus(false))
	{
		printf("\n Failed to Stop Async Schedule") ;
		DisplayStats() ;
		return false ;
	}

	if(!CheckHCActive())
	{
		printf("\n Controller has stopped !!!") ;
		DisplayStats() ;
		return false ;
	}

	return true ;
}

unsigned EHCIController::GetNoOfPortsActual()
{
	return (_pCapRegs->uiHCSParams) & 0xF ;
}

unsigned EHCIController::GetNoOfPorts()
{
	unsigned uiNoOfPorts = GetNoOfPortsActual() ;

	if(uiNoOfPorts > MAX_EHCI_ENTRIES)
	{
		printf("\n No. of Ports available on EHCI Controller is: %d", uiNoOfPorts) ;
		printf("\n This is more than max supported by system software. Using only first %d ports", MAX_EHCI_ENTRIES) ;
		uiNoOfPorts = MAX_EHCI_ENTRIES ;
	}

	return uiNoOfPorts ;
}

void EHCIController::SetupPorts()
{
	printf("\n Setup Ports") ;

	unsigned uiNoOfPorts = GetNoOfPorts() ;
	bool bPPC = (_pCapRegs->uiHCSParams & 0x10) == 0x10 ;

	printf("-> NumPorts = %d", uiNoOfPorts) ;
	if(bPPC)
		printf("-> Software PortPowerCntrl") ;
	else
		printf("-> Hardware PortPowerCtrl") ;

	unsigned i ;
	for(i = 0; i < uiNoOfPorts; i++)
	{
		unsigned* pPort = &(_pOpRegs->uiPortReg[ i ]) ;

		printf("\n Setup Port: %d. Initial Value: %x", i, *pPort) ;

		if(bPPC)
		{
			*pPort |= (1 << 12) ;
			ProcessManager::Instance().Sleep(100) ;
		}

		if((*pPort & (1 << 13)))
		{
			printf("-> EHCI do not own this port. Not setting up this port.") ;
			continue ;
		}

		// Wake up enable on OverCurrent, Disconnect and Connect
		*pPort |= (0x3 << 20) ;

		// Turn off Port Test Control and Port Indicator Control
		*pPort &= (~(0x3F << 14)) ;

		// Perform Port Reset
		*pPort = (*pPort | 0x100) & ~(0x4) ;
		ProcessManager::Instance().Sleep(200) ;
		*pPort &= (~(0x100)) ;
		ProcessManager::Instance().Sleep(500) ;

		if(!(*pPort & 0x4))
		{
			printf("-> Couldn't enable") ;
			continue ;
		}

		printf("-> Enabled") ;
		if((*pPort & 0x1))
			printf("-> High Speed Dev Cncted") ;

		printf("-> New value: %x", i, *pPort) ;
		break ;
	}
}

byte EHCIController::AsyncDoorBell()
{
	if(!(_pOpRegs->uiUSBCommand & (1 << 5)) || !(_pOpRegs->uiUSBStatus & (1 << 15)))
	{
		printf("\n Async Schedule is disabled. DoorBell HandShake cannot be performed.") ;
		return EHCIController_FAILURE ;
	}

	if(_pOpRegs->uiUSBCommand & (1 << 6))
	{
		printf("-> Already a DoorBell is in progress!") ;
		return EHCIController_FAILURE ;
	}

	_pOpRegs->uiUSBStatus |= ((1 << 5)) ;
	_pOpRegs->uiUSBCommand |= (1 << 6) ;

	bool bStatus = PollWait(&(_pOpRegs->uiUSBCommand), 6, 0) ;

	if(bStatus)
		bStatus = PollWait(&(_pOpRegs->uiUSBStatus), 5, 1) ;

	_pOpRegs->uiUSBStatus |= ((1 << 5)) ;

	if(!bStatus)
		return EHCIController_FAILURE ;

	return EHCIController_SUCCESS ;
}

byte EHCIController::Probe()
{
  RETURN_X_IF_NOT(PerformBiosToOSHandoff(), EHCIController_SUCCESS, EHCIController_FAILURE);

	_pOpRegs->uiCtrlDSSegment = 0 ;

	SetupInterrupts();

	RETURN_X_IF_NOT(SetupPeriodicFrameList(), EHCIController_SUCCESS, EHCIController_FAILURE) ;
	RETURN_X_IF_NOT(SetupAsyncList(), EHCIController_SUCCESS, EHCIController_FAILURE) ;

	SetSchedEnable(PERIODIC_SCHEDULE_ENABLE, false) ;
	SetSchedEnable(ASYNC_SCHEDULE_ENABLE, false) ;
	SetFrameListSize() ;

	Start() ;

	ProcessManager::Instance().Sleep(100) ;

  RETURN_X_IF_NOT(SetConfigFlag(true), EHCIController_SUCCESS, EHCIController_FAILURE);

	if(!CheckHCActive())
	{
		printf("\n Failed to start EHCI Host Controller") ;
		DisplayStats() ;
		return EHCIController_FAILURE ;
	}

	SetupPorts() ;

	RETURN_X_IF_NOT(StartAsyncSchedule(), true, EHCIController_FAILURE) ;

	USBDriver* pDriver = USBController::Instance().FindDriver(new EHCIDevice(*this));

	if(pDriver)
		printf("\n'%s' driver found for the USB Device", pDriver->Name().c_str());
	else
		printf("\nNo Driver found for this USB device") ;

	return EHCIController_SUCCESS ;
}

byte EHCIController::PerformBiosToOSHandoff()
{
	byte bEECPOffSet = (_pCapRegs->uiHCCParams >> 8) & 0xFF ;

	if(bEECPOffSet)
	{
		printf("\n Trying to perform complete handoff of EHCI Controller from BIOS to OS - EECP Offset: %x", bEECPOffSet) ;

		unsigned uiLegSup ;
		_pPCIEntry->ReadPCIConfig(bEECPOffSet, 4, &uiLegSup) ;

		printf("\n USB EHCI LEGSUP: %x", uiLegSup) ;
		if((uiLegSup & (1 << 24)) == 0x1)
		{
			printf(", EHCI Controller is already owned by OS. No need for Handoff") ;
			return EHCIController_SUCCESS ;
		}

		uiLegSup = uiLegSup | ( 1 << 24 ) ;

		_pPCIEntry->WritePCIConfig(bEECPOffSet, 4, uiLegSup) ;
		ProcessManager::Instance().Sleep(500) ;
		_pPCIEntry->ReadPCIConfig(bEECPOffSet, 4, &uiLegSup) ;

		printf(", New USB EHCI LEGSUP: %x", uiLegSup) ;
		if((uiLegSup & (1 << 24)) == 0x0)
		{
			printf("\n BIOS to OS Handoff failed") ;
			return EHCIController_FAILURE ;
		}
	}
	else
	{
		printf("\n EHCI: System does not support Extended Capabilities. Cannot perform Bios Handoff") ;
		return EHCIController_FAILURE ;
	}

	return EHCIController_SUCCESS ;
}

EHCIQueueHead* EHCIController::CreateDeviceQueueHead(int iMaxPacketSize, int iEndPtAddr, int iDevAddr)
{
	EHCIQueueHead* pQH = EHCIDataHandler_CreateAsyncQueueHead() ;

	pQH->uiAltpTDPointer_NAKCnt = 1 ;
	pQH->uiNextpTDPointer = 1 ;

	pQH->uiEndPointCap_Part1 = (5 << 28) | ((iMaxPacketSize & 0x7FF) << 16) | (1 << 14) | (2 << 12) | ((iEndPtAddr & 0xF) << 8) | (iDevAddr & 0x7F) ;
	pQH->uiEndPointCap_Part2 = (1 << 30) ;

	AddAsyncQueueHead(pQH) ;

	return pQH ;
}
