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
#include <UHCIDataHandler.h>

#include <stdio.h>

static const IRQ* UHCI_USB_IRQ ;

static byte UHCIController_GetDescriptor(unsigned uiIOBase, char devAddr, unsigned short usDescValue, unsigned short usIndex, int iLen, void* pDestDesc) ;
static byte UHCIController_SetAddress(unsigned uiIOBase, char devAddr) ;
static byte UHCIController_GetConfiguration(unsigned uiIOBase, char devAddr, char* bConfigValue) ;
static byte UHCIController_SetConfiguration(unsigned uiIOBase, char devAddr, char bConfigValue) ;
static byte UHCIController_CheckConfiguration(unsigned uiIOBase, char devAddr, char* bConfigValue, char bNumConfigs) ;
static byte UHCIController_GetDeviceDescriptor(unsigned uiIOBase, char devAddr, USBStandardDeviceDesc* pDevDesc) ;
static byte UHCIController_GetConfigDescriptor(unsigned uiIOBase, char devAddr, char bNumConfigs,
												USBStandardConfigDesc** pConfigDesc) ;

static byte UHCIController_Alloc(PCIEntry* pPCIEntry, unsigned uiIOBase, unsigned uiIOSize) ;
static void UHCIController_IRQHandler() ;
static byte UHCIController_GetStringDescriptorZero(unsigned uiIOBase, char devAddr, USBStringDescZero** ppStrDescZero) ;

static byte UHCIController_GetDescriptor(unsigned uiIOBase, char devAddr, unsigned short usDescValue, unsigned short usIndex, int iLen, void* pDestDesc)
{
	byte bStatus ;
	unsigned uiFrameNumber ;
	RETURN_IF_NOT(bStatus, UHCIDataHandler_GetNextFreeFrame(&uiFrameNumber), UHCIDataHandler_SUCCESS) ;

	UHCIQueueHead* pQH = UHCIDataHandler_CreateQueueHead() ;
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1) ;

	UHCITransferDesc* pTD1 = UHCIDataHandler_CreateTransferDesc() ;

	//UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
	//UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_DEVICE_ADDR, devAddr) ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_MAXLEN, 7) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_PID, TOKEN_PID_SETUP) ;
	
	if(iLen < 0)
		iLen = DEF_DESC_LEN ;

	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateAlignForKernel(sizeof(USBDevRequest), 16) ;
	pDevRequest->bRequestType = 0x80 ;
	pDevRequest->bRequest = 6 ;
	pDevRequest->usWValue = usDescValue ;
	pDevRequest->usWIndex = usIndex ;
	pDevRequest->usWLength = iLen + 1 ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest) ;
	
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1) ;
	
	UHCITransferDesc* pTD2 = UHCIDataHandler_CreateTransferDesc() ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2) ;

	unsigned uiBufPtr = DMM_AllocateAlignForKernel(iLen, 16) ;

	//UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
	//UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DEVICE_ADDR, devAddr) ;
	
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_MAXLEN, iLen - 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DATA_TOGGLE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_PID, TOKEN_PID_IN) ;

	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_BUF_PTR, uiBufPtr) ;

	UHCITransferDesc* pTD3 = UHCIDataHandler_CreateTransferDesc() ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD3) ;

	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_TERMINATE, 1) ;
	//pTD3->uiLinkPointer = 5 ;
	
	//UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_DEVICE_ADDR, devAddr) ;
	
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_PID, TOKEN_PID_OUT) ;

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n TD3 Status = %x/%x/%x", 
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken,
				(unsigned)pTD3, pTD3->uiControlnStatus, pTD3->uiToken) ;

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		printf("\nFrame Number = %d", uiFrameNumber) ;

		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		ProcessManager::Instance().Sleep(5000) ;
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n TD3 Status = %x/%x/%x", 
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken,
				(unsigned)pTD3, pTD3->uiControlnStatus, pTD3->uiToken) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
		printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG)) ;
		printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;

		void* pDesc = (void*)UHCIDataHandler_GetTDAttribute(pTD2, UHCI_ATTR_BUF_PTR) ;
		
		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pDesc, MemUtil_GetDS(), (unsigned) pDestDesc, iLen) ;

		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, UHCIDataHandler_CleanFrame(uiFrameNumber)) ;
		ProcessManager::Instance().Sleep(2000) ;
	}
	else
	{
		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;

		if(UHCIManager::Instance().PollWait(&(pQH->uiElementLinkPointer), 1) == false)
		{
			printf("\nFrame Number = %d", uiFrameNumber) ;
			printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
			printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
			printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n TD3 Status = %x/%x/%x", 
					(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
					(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken,
					(unsigned)pTD3, pTD3->uiControlnStatus, pTD3->uiToken) ;
			printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
			printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG)) ;
			printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		}

		void* pDesc = (void*)UHCIDataHandler_GetTDAttribute(pTD2, UHCI_ATTR_BUF_PTR) ;
		
		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pDesc, MemUtil_GetDS(), (unsigned) pDestDesc, iLen) ;

		UHCIDataHandler_CleanFrame(uiFrameNumber) ;
	}

	return UHCIController_SUCCESS ;
}

static byte UHCIController_GetConfiguration(unsigned uiIOBase, char devAddr, char* bConfigValue)
{
	byte bStatus ;
	unsigned uiFrameNumber ;
	RETURN_IF_NOT(bStatus, UHCIDataHandler_GetNextFreeFrame(&uiFrameNumber), UHCIDataHandler_SUCCESS) ;

	UHCIQueueHead* pQH = UHCIDataHandler_CreateQueueHead() ;
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1) ;

	UHCITransferDesc* pTD1 = UHCIDataHandler_CreateTransferDesc() ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_DEVICE_ADDR, devAddr) ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_MAXLEN, 7) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_PID, TOKEN_PID_SETUP) ;
	
	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateAlignForKernel(sizeof(USBDevRequest), 16) ;
	pDevRequest->bRequestType = 0x80 ;
	pDevRequest->bRequest = 8 ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = 0 ;
	pDevRequest->usWLength = 1 ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest) ;
	
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1) ;
	
	UHCITransferDesc* pTD2 = UHCIDataHandler_CreateTransferDesc() ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2) ;

	unsigned uiBufPtr = DMM_AllocateAlignForKernel(1, 16) ;

	//UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
	//UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DEVICE_ADDR, devAddr) ;
	
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_MAXLEN, 0) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DATA_TOGGLE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_PID, TOKEN_PID_IN) ;

	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_BUF_PTR, uiBufPtr) ;

	UHCITransferDesc* pTD3 = UHCIDataHandler_CreateTransferDesc() ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD3) ;

	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_TERMINATE, 1) ;
	
	//UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_DEVICE_ADDR, devAddr) ;
	
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_PID, TOKEN_PID_OUT) ;

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n TD3 Status = %x/%x/%x", 
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken,
				(unsigned)pTD3, pTD3->uiControlnStatus, pTD3->uiToken) ;

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		printf("\nFrame Number = %d", uiFrameNumber) ;

		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		ProcessManager::Instance().Sleep(5000) ;
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n TD3 Status = %x/%x/%x", 
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken,
				(unsigned)pTD3, pTD3->uiControlnStatus, pTD3->uiToken) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
		printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG)) ;
		printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;

		*bConfigValue = *((char*)UHCIDataHandler_GetTDAttribute(pTD2, UHCI_ATTR_BUF_PTR)) ;

		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, UHCIDataHandler_CleanFrame(uiFrameNumber)) ;
		ProcessManager::Instance().Sleep(2000) ;
	}
	else
	{
		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;

		if(UHCIManager::Instance().PollWait(&(pQH->uiElementLinkPointer), 1) == false)
		{
			printf("\nFrame Number = %d", uiFrameNumber) ;
			printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
			printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
			printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n TD3 Status = %x/%x/%x", 
					(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
					(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken,
					(unsigned)pTD3, pTD3->uiControlnStatus, pTD3->uiToken) ;

			printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
			printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG)) ;
			printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		}

		*bConfigValue = *((char*)UHCIDataHandler_GetTDAttribute(pTD2, UHCI_ATTR_BUF_PTR)) ;

		UHCIDataHandler_CleanFrame(uiFrameNumber) ;
	}

	return UHCIController_SUCCESS ;
}

static byte UHCIController_SetConfiguration(unsigned uiIOBase, char devAddr, char bConfigValue)
{
	byte bStatus ;
	unsigned uiFrameNumber ;
	RETURN_IF_NOT(bStatus, UHCIDataHandler_GetNextFreeFrame(&uiFrameNumber), UHCIDataHandler_SUCCESS) ;

	UHCIQueueHead* pQH = UHCIDataHandler_CreateQueueHead() ;
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1) ;

	UHCITransferDesc* pTD1 = UHCIDataHandler_CreateTransferDesc() ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_DEVICE_ADDR, devAddr) ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_MAXLEN, 7) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_PID, TOKEN_PID_SETUP) ;
	
	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateAlignForKernel(sizeof(USBDevRequest), 16) ;
	pDevRequest->bRequestType = 0x00 ;
	pDevRequest->bRequest = 9 ;
	pDevRequest->usWValue = bConfigValue ;
	pDevRequest->usWIndex = 0 ;
	pDevRequest->usWLength = 0 ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest) ;
	
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1) ;
	
	UHCITransferDesc* pTD2 = UHCIDataHandler_CreateTransferDesc() ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2) ;

	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_TERMINATE, 1) ;

	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DEVICE_ADDR, devAddr) ;
	
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DATA_TOGGLE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_PID, TOKEN_PID_IN) ;

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		printf("\nFrame Number = %d", uiFrameNumber) ;

		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		ProcessManager::Instance().Sleep(5000) ;
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
		printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG)) ;
		printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;

		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, UHCIDataHandler_CleanFrame(uiFrameNumber)) ;
		ProcessManager::Instance().Sleep(2000) ;
	}
	else
	{
		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;

		if(UHCIManager::Instance().PollWait(&(pQH->uiElementLinkPointer), 1) == false)
		{
			printf("\nFrame Number = %d", uiFrameNumber) ;
			printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
			printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
			printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
					(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
					(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;

			printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
			printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG)) ;
			printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		}

		UHCIDataHandler_CleanFrame(uiFrameNumber) ;
	}

	return UHCIController_SUCCESS ;
}

static byte UHCIController_CheckConfiguration(unsigned uiIOBase, char devAddr, char* bConfigValue, char bNumConfigs)
{
	if(bNumConfigs <= 0)
	{
		printf("\n Invalid NumConfigs: %d", bNumConfigs) ;
		return UHCIController_ERR_CONFIG ;
	}

	if(*bConfigValue <= 0 || *bConfigValue > bNumConfigs)
	{
		byte bStatus ;
		printf("\n Current Configuration %d -> Invalid. Setting to Configuration 1", *bConfigValue) ;
		RETURN_IF_NOT(bStatus, UHCIController_SetConfiguration(uiIOBase, devAddr, 1), UHCIController_SUCCESS) ;
		*bConfigValue = 1 ;
	}

	return UHCIController_SUCCESS ;
}

static byte UHCIController_SetAddress(unsigned uiIOBase, char devAddr)
{
	byte bStatus ;
	unsigned uiFrameNumber ;
	RETURN_IF_NOT(bStatus, UHCIDataHandler_GetNextFreeFrame(&uiFrameNumber), UHCIDataHandler_SUCCESS) ;

	UHCIQueueHead* pQH = UHCIDataHandler_CreateQueueHead() ;
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1) ;

	UHCITransferDesc* pTD1 = UHCIDataHandler_CreateTransferDesc() ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_MAXLEN, 7) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_PID, TOKEN_PID_SETUP) ;
	
	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateAlignForKernel(sizeof(USBDevRequest), 16) ;
	pDevRequest->bRequestType = 0x00 ;
	pDevRequest->bRequest = 5 ;
	pDevRequest->usWValue = devAddr ;
	pDevRequest->usWIndex = 0 ;
	pDevRequest->usWLength = 0 ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest) ;
	
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1) ;
	
	UHCITransferDesc* pTD2 = UHCIDataHandler_CreateTransferDesc() ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2) ;

	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_TERMINATE, 1) ;

	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DATA_TOGGLE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_PID, TOKEN_PID_IN) ;

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		printf("\nFrame Number = %d", uiFrameNumber) ;

		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		ProcessManager::Instance().Sleep(5000) ;
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
		printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG)) ;
		printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;

		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, UHCIDataHandler_CleanFrame(uiFrameNumber)) ;
		ProcessManager::Instance().Sleep(2000) ;
	}
	else
	{
		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;

		if(UHCIManager::Instance().PollWait(&(pQH->uiElementLinkPointer), 1) == false)
		{
			printf("\nFrame Number = %d", uiFrameNumber) ;
			printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
			printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
			printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
					(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
					(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;

			printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
			printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG)) ;
			printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		}

		UHCIDataHandler_CleanFrame(uiFrameNumber) ;
	}

	return UHCIController_SUCCESS ;
}

static byte UHCIController_GetDeviceDescriptor(unsigned uiIOBase, char devAddr, USBStandardDeviceDesc* pDevDesc)
{
	byte bStatus ;

	USBDataHandler_InitDevDesc(pDevDesc) ;

	RETURN_IF_NOT(bStatus, UHCIController_GetDescriptor(uiIOBase, devAddr, 0x100, 0, -1, pDevDesc), UHCIDataHandler_SUCCESS) ;

	byte iLen = pDevDesc->bLength ;

	if(iLen >= sizeof(USBStandardDeviceDesc))
		iLen = sizeof(USBStandardDeviceDesc) ;

	RETURN_IF_NOT(bStatus, UHCIController_GetDescriptor(uiIOBase, devAddr, 0x100, 0, iLen, pDevDesc), UHCIDataHandler_SUCCESS) ;

	return UHCIController_SUCCESS ;
}

static byte UHCIController_GetConfigDescriptor(unsigned uiIOBase, char devAddr, char bNumConfigs, USBStandardConfigDesc** pConfigDesc)
{
	byte bStatus = UHCIController_SUCCESS ;
	int index ;

	USBStandardConfigDesc* pCD = (USBStandardConfigDesc*)DMM_AllocateForKernel(sizeof(USBStandardConfigDesc) * bNumConfigs) ;

	for(index = 0; index < (int)bNumConfigs; index++)
		USBDataHandler_InitConfigDesc(&pCD[index]) ;

	for(index = 0; index < (int)bNumConfigs; index++)
	{
		unsigned short usDescValue = (0x2 << 8) | (index & 0xFF) ;

		bStatus = UHCIController_GetDescriptor(uiIOBase, devAddr, usDescValue, 0, -1, &(pCD[index])) ;

		if(bStatus != UHCIController_SUCCESS)
			break ;

		int iLen = pCD[index].wTotalLength;

		void* pBuffer = (void*)DMM_AllocateForKernel(iLen) ;

		bStatus = UHCIController_GetDescriptor(uiIOBase, devAddr, usDescValue, 0, iLen, pBuffer) ;

		if(bStatus != UHCIController_SUCCESS)
		{
			DMM_DeAllocateForKernel((unsigned)pBuffer) ;
			break ;
		}

		USBDataHandler_CopyConfigDesc(&pCD[index], pBuffer, pCD[index].bLength) ;

		USBDataHandler_DisplayConfigDesc(&pCD[index]) ;

		printf("\n Parsing Interface information for Configuration: %d", index) ;
		printf("\n Number of Interfaces: %d", (int)pCD[ index ].bNumInterfaces) ;
		ProcessManager::Instance().Sleep(10000) ;

		void* pInterfaceBuffer = (char*)pBuffer + pCD[index].bLength ;

		pCD[index].pInterfaces = (USBStandardInterface*)DMM_AllocateForKernel(sizeof(USBStandardInterface) * pCD[index].bNumInterfaces) ;
		int iI ;

		for(iI = 0; iI < (int)pCD[index].bNumInterfaces; iI++)
		{
			USBDataHandler_InitInterfaceDesc(&(pCD[index].pInterfaces[iI])) ;
		}

		for(iI = 0; iI < (int)pCD[index].bNumInterfaces; iI++)
		{
			USBStandardInterface* pInt = (USBStandardInterface*)(pInterfaceBuffer) ;

			int iIntLen = sizeof(USBStandardInterface) - sizeof(USBStandardEndPt*) ;
			int iCopyLen = pInt->bLength ;

			if(iCopyLen > iIntLen)
				iCopyLen = iIntLen ;

			MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pInt, 
								MemUtil_GetDS(), (unsigned)&(pCD[index].pInterfaces[iI]),
								iCopyLen) ;

			USBDataHandler_DisplayInterfaceDesc(&(pCD[index].pInterfaces[iI])) ;

			USBStandardEndPt* pEndPtBuffer = (USBStandardEndPt*)((char*)pInterfaceBuffer + pInt->bLength) ;

			int iNumEndPoints = pCD[index].pInterfaces[iI].bNumEndpoints ;

			pCD[index].pInterfaces[iI].pEndPoints = (USBStandardEndPt*)DMM_AllocateForKernel(
														sizeof(USBStandardEndPt) * iNumEndPoints) ;

			printf("\n Parsing EndPoints for Interface: %d of Configuration: %d", iI, index) ;
			ProcessManager::Instance().Sleep(10000) ;

			int iE ;
			for(iE = 0; iE < iNumEndPoints; iE++)
				USBDataHandler_InitEndPtDesc(&(pCD[index].pInterfaces[iI].pEndPoints[iE])) ;

			for(iE = 0; iE < iNumEndPoints; iE++)
			{
				USBStandardEndPt* pEndPt = &pEndPtBuffer[iE] ;

				int iELen = sizeof(USBStandardEndPt) ;
				int iECopyLen = pEndPt->bLength ;
				
				if(iECopyLen > iELen)
					iECopyLen = iELen ;

				MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pEndPt, 
									MemUtil_GetDS(), (unsigned)&(pCD[index].pInterfaces[iI].pEndPoints[iE]),
									iECopyLen) ;

				USBDataHandler_DisplayEndPtDesc(&(pCD[index].pInterfaces[iI].pEndPoints[iE])) ;
			}

			pInterfaceBuffer = (USBStandardInterface*)((char*)pEndPtBuffer + iNumEndPoints * sizeof(USBStandardEndPt)) ;
		}

		DMM_DeAllocateForKernel((unsigned)pBuffer) ;
		pBuffer = NULL ;
	}

	if(bStatus != UHCIController_SUCCESS)
	{
		USBDataHandler_DeAllocConfigDesc(pCD, bNumConfigs) ;
		return bStatus ;
	}

	*pConfigDesc = pCD ;

	return UHCIController_SUCCESS ;
}

static byte UHCIController_GetStringDescriptorZero(unsigned uiIOBase, char devAddr, USBStringDescZero** ppStrDescZero)
{
	unsigned short usDescValue = (0x3 << 8) ;
	char szPart[ 8 ];

	byte bStatus = UHCIController_GetDescriptor(uiIOBase, devAddr, usDescValue, 0, -1, szPart) ;

	if(bStatus != UHCIController_SUCCESS)
		return bStatus ;

	int iLen = ((USBStringDescZero*)&szPart)->bLength ;
	printf("\n String Desc Zero Len: %d", iLen) ;

	byte* pStringDescZero = (byte*)DMM_AllocateForKernel(iLen) ;

	bStatus = UHCIController_GetDescriptor(uiIOBase, devAddr, usDescValue, 0, iLen, pStringDescZero) ;

	if(bStatus != UHCIController_SUCCESS)
	{
		DMM_DeAllocateForKernel((unsigned)pStringDescZero) ;
		return bStatus ;
	}

	*ppStrDescZero = (USBStringDescZero*)DMM_AllocateForKernel(sizeof(USBStringDescZero)) ;

	USBDataHandler_CopyStrDescZero(*ppStrDescZero, pStringDescZero) ;
	USBDataHandler_DisplayStrDescZero(*ppStrDescZero) ;

	DMM_DeAllocateForKernel((unsigned)pStringDescZero) ;

	return UHCIController_SUCCESS ;
}

bool UHCIDevice::GetMaxLun(byte* bLUN)
{
	byte bStatus ;
	unsigned uiFrameNumber ;
	RETURN_IF_NOT(bStatus, UHCIDataHandler_GetNextFreeFrame(&uiFrameNumber), UHCIDataHandler_SUCCESS) ;

	UHCIQueueHead* pQH = UHCIDataHandler_CreateQueueHead() ;
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1) ;

	UHCITransferDesc* pTD1 = UHCIDataHandler_CreateTransferDesc() ;

	//UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
	//UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_DEVICE_ADDR, _devAddr) ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_MAXLEN, 7) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_PID, TOKEN_PID_SETUP) ;
	
	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateAlignForKernel(sizeof(USBDevRequest), 16) ;
	pDevRequest->bRequestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN ;
	pDevRequest->bRequest = 0xFE ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = _bInterfaceNumber ;
	pDevRequest->usWLength = 1 ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest) ;
	
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1) ;
	
	UHCITransferDesc* pTD2 = UHCIDataHandler_CreateTransferDesc() ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2) ;
	
	byte* pBufPtr = (byte*)DMM_AllocateAlignForKernel(1, 16) ;

	//UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_BUF_PTR, (unsigned)pBufPtr) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DATA_TOGGLE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DEVICE_ADDR, _devAddr) ;
	
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_MAXLEN, 0) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_PID, TOKEN_PID_IN) ;
	
	UHCITransferDesc* pTD3 = UHCIDataHandler_CreateTransferDesc() ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD3) ;
	
	//UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_DEVICE_ADDR, _devAddr) ;
	
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_PID, TOKEN_PID_OUT) ;
	UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_TERMINATE, 1) ;

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x", (unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken) ;
		printf("\n TD2 Status = %x/%x/%x", (unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;
		printf("\n TD3 Status = %x/%x/%x", (unsigned)pTD3, pTD3->uiControlnStatus, pTD3->uiToken) ;

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		printf("\nFrame Number = %d", uiFrameNumber) ;

		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		ProcessManager::Instance().Sleep(5000) ;
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
		printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG + 2)) ;
		printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;

		*bLUN = pBufPtr[0] ;
		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, UHCIDataHandler_CleanFrame(uiFrameNumber)) ;
		ProcessManager::Instance().Sleep(2000) ;
	}
	else
	{
		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;

		if(UHCIManager::Instance().PollWait(&(pQH->uiElementLinkPointer), 1) == false)
		{
			printf("\nFrame Number = %d", uiFrameNumber) ;
			printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
			printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
			printf("\n TD1 Status = %x/%x/%x", (unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken) ;
			printf("\n TD2 Status = %x/%x/%x", (unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;
			printf("\n TD3 Status = %x/%x/%x", (unsigned)pTD3, pTD3->uiControlnStatus, pTD3->uiToken) ;
			printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
			printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG + 2)) ;
			printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		}
		else
			*bLUN = pBufPtr[0] ;

		UHCIDataHandler_CleanFrame(uiFrameNumber) ;
	}

	return true ;
}

bool UHCIDevice::CommandReset()
{
	byte bStatus ;
	unsigned uiFrameNumber ;
	RETURN_IF_NOT(bStatus, UHCIDataHandler_GetNextFreeFrame(&uiFrameNumber), UHCIDataHandler_SUCCESS) ;

	UHCIQueueHead* pQH = UHCIDataHandler_CreateQueueHead() ;
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1) ;

	UHCITransferDesc* pTD1 = UHCIDataHandler_CreateTransferDesc() ;

	//UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
	//UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_DEVICE_ADDR, _devAddr) ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_MAXLEN, 7) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_PID, TOKEN_PID_SETUP) ;
	
	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateAlignForKernel(sizeof(USBDevRequest), 16) ;
	pDevRequest->bRequestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE ;
	pDevRequest->bRequest = 0xFF ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = _bInterfaceNumber ;
	pDevRequest->usWLength = 0 ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest) ;
	
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1) ;
	
	UHCITransferDesc* pTD2 = UHCIDataHandler_CreateTransferDesc() ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2) ;

	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_TERMINATE, 1) ;
	
	//UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DATA_TOGGLE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DEVICE_ADDR, _devAddr) ;
	
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_PID, TOKEN_PID_OUT) ;

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		printf("\nFrame Number = %d", uiFrameNumber) ;

		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		ProcessManager::Instance().Sleep(5000) ;
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
		printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG + 2)) ;
		printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;

		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, UHCIDataHandler_CleanFrame(uiFrameNumber)) ;
		ProcessManager::Instance().Sleep(2000) ;
	}
	else
	{
		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;

		if(UHCIManager::Instance().PollWait(&(pQH->uiElementLinkPointer), 1) == false)
		{
			printf("\nFrame Number = %d", uiFrameNumber) ;
			printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
			printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
			printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
					(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
					(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;
			printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
			printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG + 2)) ;
			printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		}

		UHCIDataHandler_CleanFrame(uiFrameNumber) ;
	}

	return true ;
}

bool UHCIDevice::ClearHaltEndPoint(USBulkDisk* pDisk, bool bIn)
{
	unsigned uiEndPoint = (bIn) ? pDisk->uiEndPointIn : pDisk->uiEndPointOut ;

	byte bStatus ;
	unsigned uiFrameNumber ;
	RETURN_IF_NOT(bStatus, UHCIDataHandler_GetNextFreeFrame(&uiFrameNumber), UHCIDataHandler_SUCCESS) ;

	UHCIQueueHead* pQH = UHCIDataHandler_CreateQueueHead() ;
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1) ;

	UHCITransferDesc* pTD1 = UHCIDataHandler_CreateTransferDesc() ;

	//UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
	//UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_DEVICE_ADDR, _devAddr) ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_MAXLEN, 7) ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_PID, TOKEN_PID_SETUP) ;
	
	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateAlignForKernel(sizeof(USBDevRequest), 16) ;
	pDevRequest->bRequestType = USB_RECIP_ENDPOINT ;
	pDevRequest->bRequest = 1 ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = (uiEndPoint & 0xF) | ((bIn) ? 0x80 : 0x00) ;
	pDevRequest->usWLength = 0 ;

	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest) ;
	
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1) ;
	
	UHCITransferDesc* pTD2 = UHCIDataHandler_CreateTransferDesc() ;
	UHCIDataHandler_SetTDAttribute(pTD1, UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2) ;

	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_TERMINATE, 1) ;
	
	//UHCIDataHandler_SetTDAttribute(pTD3, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DATA_TOGGLE, 1) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_DEVICE_ADDR, _devAddr) ;
	
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK) ;
	UHCIDataHandler_SetTDAttribute(pTD2, UHCI_ATTR_TD_PID, (bIn) ? TOKEN_PID_IN : TOKEN_PID_OUT) ;

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		printf("\nFrame Number = %d", uiFrameNumber) ;

		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		ProcessManager::Instance().Sleep(5000) ;
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
				(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;
		printf("\nFrame Reg = %x", UHCIDataHandler_GetFrameListEntry(uiFrameNumber)) ;

		printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
		printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG)) ;
		printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;

		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, UHCIDataHandler_CleanFrame(uiFrameNumber)) ;
		ProcessManager::Instance().Sleep(2000) ;
	}
	else
	{
		UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true) ;

		if(UHCIManager::Instance().PollWait(&(pQH->uiElementLinkPointer), 1) == false)
		{
			printf("\nFrame Number = %d", uiFrameNumber) ;
			printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
			printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;
			printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
					(unsigned)pTD1, pTD1->uiControlnStatus, pTD1->uiToken,
					(unsigned)pTD2, pTD2->uiControlnStatus, pTD2->uiToken) ;
			printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
			printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG)) ;
			printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
		}

		if(bIn) pDisk->bEndPointInToggle = 0 ;
		else pDisk->bEndPointOutToggle = 0 ;

		UHCIDataHandler_CleanFrame(uiFrameNumber) ;
	}

	return true ;
}

bool UHCIDevice::BulkRead(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
	if(uiLen == 0)
		return true ;

	unsigned uiMaxLen = pDisk->usInMaxPacketSize * MAX_UHCI_TD_PER_BULK_RW ;
	if(uiLen > uiMaxLen)
	{
		printf("\n Max Bulk Transfer per Frame is %d bytes", uiMaxLen) ;
		return false ;
	}

	unsigned uiMaxPacketSize = pDisk->usInMaxPacketSize ;
	int iNoOfTDs = uiLen / uiMaxPacketSize ;
	iNoOfTDs += ((uiLen % uiMaxPacketSize) != 0) ;

	if(iNoOfTDs > MAX_UHCI_TD_PER_BULK_RW)
	{
		printf("\n No. of TDs per Bulk Read/Wrtie exceeded limit %d !!", MAX_UHCI_TD_PER_BULK_RW) ;
		return false ;
	}

	if(_bFirstBulkRead)
	{
		_bFirstBulkRead = false ;

		int i ;
		for(i = 0; i < MAX_UHCI_TD_PER_BULK_RW; i++)
			_ppBulkReadTDs[ i ] = UHCIDataHandler_CreateTransferDesc() ; 
	}
	else
	{
		int i ;
		for(i = 0; i < MAX_UHCI_TD_PER_BULK_RW; i++)
			UHCIDataHandler_ClearTransferDesc(_ppBulkReadTDs[ i ]) ;
	}

	int iIndex ;

	byte bStatus ;
	unsigned uiFrameNumber ;
	RETURN_IF_NOT(bStatus, UHCIDataHandler_GetNextFreeFrame(&uiFrameNumber), UHCIDataHandler_SUCCESS) ;
	UHCIQueueHead* pQH = UHCIDataHandler_CreateQueueHead() ;
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1) ;

	unsigned uiYetToRead = uiLen ;
	unsigned uiCurReadLen ;

	for(iIndex = 0; iIndex < iNoOfTDs; iIndex++)
	{
		uiCurReadLen = (uiYetToRead > uiMaxPacketSize) ? uiMaxPacketSize : uiYetToRead ;
		uiYetToRead -= uiCurReadLen ;

		UHCITransferDesc* pTD = _ppBulkReadTDs[ iIndex ] ;

		//UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_DEVICE_ADDR, _devAddr) ;
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_ENDPT_ADDR, pDisk->uiEndPointIn) ;
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_DATA_TOGGLE, pDisk->bEndPointInToggle) ;
		pDisk->bEndPointInToggle ^= 1 ;

		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_MAXLEN, uiCurReadLen - 1) ;
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_PID, TOKEN_PID_IN) ;
		
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_BUF_PTR, (unsigned)(pDisk->pRawAlignedBuffer + (iIndex * uiMaxPacketSize))) ;

		if(iIndex > 0)
			UHCIDataHandler_SetTDAttribute(_ppBulkReadTDs[ iIndex - 1 ], UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD) ;
	}

	UHCIDataHandler_SetTDAttribute(_ppBulkReadTDs[ iIndex - 1 ], UHCI_ATTR_TD_TERMINATE, 1) ;
	UHCIDataHandler_SetTDAttribute(_ppBulkReadTDs[ iIndex - 1 ], UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)(_ppBulkReadTDs[ 0 ])) ;

	UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, false, false) ;

	if(UHCIManager::Instance().PollWait(&(pQH->uiElementLinkPointer), 1) == false)
	{
		printf("\nFrame Number = %d", uiFrameNumber) ;
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;

		for(iIndex = 0; iIndex < iNoOfTDs; iIndex++)
      printf("\n TD%d Status = %x/%x/%x", iIndex+1, (unsigned)(_ppBulkReadTDs[ iIndex ]), _ppBulkReadTDs[ iIndex ]->uiControlnStatus, _ppBulkReadTDs[ iIndex ]->uiToken);

     printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
     printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG)) ;
     printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
   }

   memcpy(pDataBuf, pDisk->pRawAlignedBuffer, uiLen) ;
   UHCIDataHandler_CleanFrame(uiFrameNumber) ;

   return true ;
}

bool UHCIDevice::BulkWrite(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
  if(uiLen == 0)
    return true ;

  unsigned uiMaxLen = pDisk->usOutMaxPacketSize * MAX_UHCI_TD_PER_BULK_RW ;
  if(uiLen > uiMaxLen)
  {
     printf("\n Max Bulk Transfer per Frame is %d bytes", uiMaxLen) ;
     return false ;
  }

  unsigned uiMaxPacketSize = pDisk->usOutMaxPacketSize ;
  int iNoOfTDs = uiLen / uiMaxPacketSize ;
  iNoOfTDs += ((uiLen % uiMaxPacketSize) != 0) ;

  if(iNoOfTDs > MAX_UHCI_TD_PER_BULK_RW)
  {
     printf("\n No. of TDs per Bulk Read/Wrtie exceeded limit %d !!", MAX_UHCI_TD_PER_BULK_RW) ;
     return false ;
  }

  if(_bFirstBulkWrite)
  {
    _bFirstBulkWrite = false ;
    for(int i = 0; i < MAX_UHCI_TD_PER_BULK_RW; i++)
      _ppBulkWriteTDs[ i ] = UHCIDataHandler_CreateTransferDesc() ; 
  }
  else
  {
    for(int i = 0; i < MAX_UHCI_TD_PER_BULK_RW; i++)
      UHCIDataHandler_ClearTransferDesc(_ppBulkWriteTDs[ i ]) ;
  }

  int iIndex ;

  byte bStatus ;
  unsigned uiFrameNumber ;
  RETURN_IF_NOT(bStatus, UHCIDataHandler_GetNextFreeFrame(&uiFrameNumber), UHCIDataHandler_SUCCESS) ;
  UHCIQueueHead* pQH = UHCIDataHandler_CreateQueueHead() ;

  UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1) ;

  memcpy(pDisk->pRawAlignedBuffer, pDataBuf, uiLen) ;

  unsigned uiYetToWrite = uiLen ;
  unsigned uiCurWriteLen ;

  for(iIndex = 0; iIndex < iNoOfTDs; iIndex++)
	{
		uiCurWriteLen = (uiYetToWrite > uiMaxPacketSize) ? uiMaxPacketSize : uiYetToWrite ;
		uiYetToWrite -= uiCurWriteLen ;

		UHCITransferDesc* pTD = _ppBulkWriteTDs[ iIndex ] ;

		//UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_CONTROL_SPD, 1) ;
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3) ;
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_STATUS_ACTIVE, 1) ;
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_DEVICE_ADDR, _devAddr) ;
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_ENDPT_ADDR, pDisk->uiEndPointOut) ;
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_DATA_TOGGLE, pDisk->bEndPointOutToggle) ;
		pDisk->bEndPointOutToggle ^= 1 ;

		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_MAXLEN, uiCurWriteLen - 1) ;
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_TD_PID, TOKEN_PID_OUT) ;
		
		UHCIDataHandler_SetTDAttribute(pTD, UHCI_ATTR_BUF_PTR, (unsigned)(pDisk->pRawAlignedBuffer + (iIndex * uiMaxPacketSize))) ;

		if(iIndex > 0)
			UHCIDataHandler_SetTDAttribute(_ppBulkWriteTDs[ iIndex - 1 ], UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD) ;
	}

	UHCIDataHandler_SetTDAttribute(_ppBulkWriteTDs[ iIndex - 1 ], UHCI_ATTR_TD_TERMINATE, 1) ;
	UHCIDataHandler_SetTDAttribute(_ppBulkWriteTDs[ iIndex - 1 ], UHCI_ATTR_TD_CONTROL_IOC, 1) ;
	UHCIDataHandler_SetQHAttribute(pQH, UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)(_ppBulkWriteTDs[ 0 ])) ;
	
	UHCIDataHandler_SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, false, false) ;

	if(UHCIManager::Instance().PollWait(&(pQH->uiElementLinkPointer), 1) == false)
	{
		printf("\nFrame Number = %d", uiFrameNumber) ;
		printf("\n QH Element Pointer = %x", pQH->uiElementLinkPointer) ;
		printf("\n QH Head Pointer = %x", pQH->uiHeadLinkPointer) ;

		for(iIndex = 0; iIndex < iNoOfTDs; iIndex++)
			printf("\n TD%d Status = %x/%x/%x", iIndex+1, (unsigned)(_ppBulkWriteTDs[ iIndex ]), 
					_ppBulkWriteTDs[ iIndex ]->uiControlnStatus, _ppBulkWriteTDs[ iIndex ]->uiToken) ;

		printf("\n Status = %x", PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) ;
		printf("\n Port Status = %x", PortCom_ReceiveWord(uiIOBase + PORTSC_REG)) ;
		printf("\nFrame Number = %d", PortCom_ReceiveWord(uiIOBase + FRNUM_REG)) ;
	}

	UHCIDataHandler_CleanFrame(uiFrameNumber) ;

	return true ;
}

static byte UHCIController_Alloc(PCIEntry* pPCIEntry, unsigned uiIOBase, unsigned uiIOSize)
{
	int iIRQ = pPCIEntry->BusEntity.NonBridge.bInterruptLine ;

	KC::MDisplay().Address("\n IO Addr = ", uiIOBase) ;
	KC::MDisplay().Address("\n IO Size = ", uiIOSize) ;
	
	KC::MDisplay().Number("\n USB UHCI at I/O: ", uiIOBase) ;
	KC::MDisplay().Number(", IRQ: ", iIRQ) ;

	UHCI_USB_IRQ = PIC::Instance().RegisterIRQ(iIRQ, (unsigned)&UHCIController_IRQHandler) ;
	if(!UHCI_USB_IRQ)
		return UHCIController_FAILURE ;
	
	// As per UCHI Spec, there should be minimum 2 Ports present
	// 8th Bit from LSB is always set and that is used as a termination condition
	// of the Port iteration loop
	
	int iNumPorts ;
	int iPossibleNumPorts = ( uiIOSize - PORTSC_REG ) / 2 ;

	for(iNumPorts = 0; iNumPorts < iPossibleNumPorts; iNumPorts++)
	{
		unsigned uiPortAddr = uiIOBase + PORTSC_REG + iNumPorts * 2 ;

		unsigned short usPortStatus = PortCom_ReceiveWord(uiPortAddr) ;

		printf("\n USB Port: %d, Addr: %x, Status: %x", iNumPorts, uiPortAddr, usPortStatus) ;

		if(!(usPortStatus & 0x0080))
			break ;
	}

	KC::MDisplay().Number("\n Detected Ports: ", iNumPorts) ;

	if(iNumPorts < 2 || iNumPorts > 8)
		iNumPorts = 2 ;

	//Reset Hub
	/* Global reset for 50ms */
	PortCom_SendWord(uiIOBase + USBCMD_REG, USBCMD_GRESET) ;
	ProcessManager::Instance().Sleep(100) ;
	PortCom_SendWord(uiIOBase + USBCMD_REG, 0) ;
	ProcessManager::Instance().Sleep(100) ;

	//Start Hub
	/* Reset the HC - this will force us to get a new notification of any
	 * already connected ports due to the virtual disconnect that it
	 * implies.
	 */
	unsigned uiLimit = 1000;
	PortCom_SendWord(uiIOBase + USBCMD_REG, USBCMD_HCRESET) ;
	ProcessManager::Instance().Sleep(50) ;
	
	while(PortCom_ReceiveWord(uiIOBase + USBCMD_REG) & USBCMD_HCRESET)
	{
		// Spec says we should wait for 10ms before HCRESET is set to 0
		if(uiLimit == 1)
			ProcessManager::Instance().Sleep(10) ;

		if(! (--uiLimit) )
		{
			KC::MDisplay().Message("\n USBCMD_HCRESET TimeOut!", ' ') ;
			break;
		}
	}

	/* Start at frame 0 */
	PortCom_SendWord(uiIOBase + FRNUM_REG, 0) ;

	unsigned uiFrameListAddr = UHCIDataHandler_CreateFrameList() ;
	PortCom_SendDoubleWord(uiIOBase + FLBASEADD_REG, uiFrameListAddr + GLOBAL_DATA_SEGMENT_BASE) ;

	/* Turn on all interrupts */
	PortCom_SendWord(uiIOBase + USBINTR_REG, USBINTR_SHORT_PACKET | USBINTR_IOC | USBINTR_RESUME | USBINTR_TIMEOUT_CRC) ;

	/* Run and mark it configured with a 64-byte max packet */
	PortCom_SendWord(uiIOBase + USBCMD_REG, (USBCMD_RS | USBCMD_CF | USBCMD_MAXP)) ;

	ProcessManager::Instance().Sleep(200) ;

	// Check Status
	// For the time just checking for zero
	unsigned short usWord = 0;
	if((usWord = PortCom_ReceiveWord(uiIOBase + USBSTS_REG)) != 0x0)
	{
		KC::MDisplay().Address("\n Error. UHCI Host Controller Status = ", usWord) ;
		usWord = PortCom_ReceiveWord(uiIOBase + USBCMD_REG) ;
		printf("\n CMD: %x", usWord) ;

		usWord = PortCom_ReceiveWord(uiIOBase + FRNUM_REG) ;
		printf("\n FRNUM: %x", usWord) ;

		unsigned uiWord = PortCom_ReceiveDoubleWord(uiIOBase + FLBASEADD_REG) ;
		printf("\n FR BASE: %x", uiWord) ;

		usWord = PortCom_ReceiveWord(uiIOBase + USBINTR_REG) ;
		printf("\n INT: %x", usWord) ;

		return UHCIController_FAILURE ;
	}

	// Consistency Check
	// Assert for Default value of SOF register
	usWord = 0;
	if((usWord = PortCom_ReceiveByte(uiIOBase + SOF_REG)) != 0x40)
	{
		KC::MDisplay().Address("\n Default of SOF register is not 0x40. --> ", usWord) ;
		return UHCIController_FAILURE ;
	}
	// End of Start Hub

	/* Enable PIRQ - Legacy Mouse and Keyboard Support for 8042 */
	pPCIEntry->WritePCIConfig(USB_LEGSUP, 2, USB_LEGSUP_DEFAULT) ;

	printf("\n USB UHCI at I/O: %x, IRQ: %d, Detected Ports: %d ", uiIOBase, iIRQ, iNumPorts) ;

	ProcessManager::Instance().Sleep(100) ;

	bool bActivePortFound = false ;
	for(int i = 0; i < iNumPorts; i++)
	{
		unsigned uiPortAddr = uiIOBase + PORTSC_REG + i * 2 ;
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
			bActivePortFound = true ;
	}

	if(!bActivePortFound)
		return UHCIController_FAILURE ;

	UHCIDataHandler_StartFrameCleaner() ;

	USBDriver* pDriver = USBController::Instance().FindDriver(new UHCIDevice());

	if(pDriver)
		printf("\n'%s' driver found for the USB Device\n", pDriver->Name().c_str()) ;
	else
		printf("\nNo Driver found for this USB device\n") ;
	
	return UHCIController_SUCCESS ;
}

static void UHCIController_IRQHandler()
{
	unsigned GPRStack[NO_OF_GPR] ;
	AsmUtil_STORE_GPR(GPRStack) ;
	AsmUtil_SET_KERNEL_DATA_SEGMENTS

	KC::MDisplay().Message("\n USB IRQ \n", ' ') ;
	//ProcessManager_SignalInterruptOccured(HD_PRIMARY_IRQ) ;

	PIC::Instance().SendEOI(*UHCI_USB_IRQ) ;
	
	AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
	AsmUtil_RESTORE_GPR(GPRStack) ;
	
	asm("leave") ;
	asm("IRET") ;
}

/****************************************************************************/

UHCIManager::UHCIManager()
{
	for(auto pPCIEntry : PCIBusHandler::Instance().PCIEntries())
	{
		if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE)
			continue ;

		if(pPCIEntry->bInterface == 0x0 && 
			pPCIEntry->bClassCode == PCI_SERIAL_BUS && 
			pPCIEntry->bSubClass == PCI_USB)
		{
      printf("\n Interface = %u, Class = %u, SubClass = %u", pPCIEntry->bInterface, pPCIEntry->bClassCode, pPCIEntry->bSubClass);
      _uhciEntries.push_back(pPCIEntry);
		}
	}
	
	if(_uhciEntries.size())
		KC::MDisplay().LoadMessage("USB UHCI Controller Found", Success) ;
	else
		KC::MDisplay().LoadMessage("No USB UHCI Controller Found", Failure) ;
}

byte UHCIManager::ProbeDevice()
{
	if(_uhciEntries.empty())
	{
    printf("\n No USB UHCI Controller Found on PCI Bus");
		return UHCIController_ERR_NOCNTL_FOUND;
	}
	
	for(auto pPCIEntry : _uhciEntries)
	{
		if(!pPCIEntry->BusEntity.NonBridge.bInterruptLine)
		{
			KC::MDisplay().Message("UHCI device with no IRQ. Check BIOS/PCI settings!", ' ');
			continue ;
			//return UHCIController_FAILURE ;
		}

		/* Enable busmaster */
		unsigned short usCommand ;
		pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand);

		pPCIEntry->WritePCIConfig(PCI_COMMAND, 2, usCommand | PCI_COMMAND_IO | PCI_COMMAND_MASTER) ;
		
		/* Search for the IO base address */
		int i ;
		unsigned* pBaseAddress = &(pPCIEntry->BusEntity.NonBridge.uiBaseAddress0) ;
		for(i = 0; i < 6; i++)
		{
			unsigned uiIOBase = pBaseAddress[i] ;

			if(!(uiIOBase & PCI_IO_ADDRESS_SPACE))
				continue ;

			unsigned uiIOSize = pPCIEntry->GetPCIMemSize(i) ;

			printf("\n Bus: %d, Device: %d, Function: %d", pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction) ;

			/* disable legacy emulation */
			pPCIEntry->WritePCIConfig(USB_LEGSUP, 2, 0) ;
			//pPCIEntry->WritePCIConfig(USB_LEGSUP, 2, 0x100) ;
			unsigned short usCommand ;
			pPCIEntry->ReadPCIConfig(USB_LEGSUP, 2, &usCommand) ;
			printf("\n USB LEGSUP: %x", usCommand) ;
//
			pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand) ;
			printf("\n PCI COMMAND: %x", usCommand) ;
  			//ProcessManager::Instance().Sleep(5000) ;

			if(UHCIController_Alloc(pPCIEntry, uiIOBase & PCI_ADDRESS_IO_MASK, uiIOSize) == UHCIController_SUCCESS)
				return UHCIController_SUCCESS ;
		}
	}

	return UHCIController_FAILURE ;
}

bool UHCIManager::PollWait(unsigned* pPoleAddr, unsigned uiValue)
{
	// 50 ms * 200 = 10 seconds
	int iTotalAttempts = 200 ;

	while(true)
	{
		if(iTotalAttempts <= 0)
			return false ;

		if((*pPoleAddr) == uiValue)
			break ;

		ProcessManager::Instance().Sleep(50) ;
	
		iTotalAttempts-- ;
	}

	return true ;
}

UHCIDevice::UHCIDevice() : _bFirstBulkRead(true), _bFirstBulkWrite(true)
{
	_devAddr = USBController::Instance().GetNextDevNum() ;
	if(_devAddr < -1)
    throw upan::exception(XLOC, "Maximum USB Device Limit reached. New Device Allocation Failed!");
	
	UHCIController_SetAddress(uiIOBase, _devAddr) ;

	USBStandardDeviceDesc devDesc ;
	UHCIController_GetDeviceDescriptor(uiIOBase, _devAddr, &devDesc) ;
	USBDataHandler_DisplayDevDesc(&devDesc) ;

	char bConfigValue = 0;
	UHCIController_GetConfiguration(uiIOBase, _devAddr, &bConfigValue) ;

  if(UHCIController_CheckConfiguration(uiIOBase, _devAddr, &bConfigValue, devDesc.bNumConfigs) != UHCIController_SUCCESS)
    throw upan::exception(XLOC, "failed to CheckConfiguration");

  if(UHCIController_GetConfigDescriptor(uiIOBase, _devAddr, devDesc.bNumConfigs, &_pArrConfigDesc) != UHCIController_SUCCESS)
    throw upan::exception(XLOC, "failed to ConfigDescriptor");

  if(UHCIController_GetStringDescriptorZero(uiIOBase, _devAddr, &_pStrDescZero) != UHCIController_SUCCESS)
    throw upan::exception(XLOC, "failed to StringDescriptorZero");

	_eControllerType = UHCI_CONTROLLER ;
	USBDataHandler_CopyDevDesc(&_deviceDesc, &devDesc, sizeof(USBStandardDeviceDesc)) ;

	SetLangId();

	_iConfigIndex = 0 ;

	for(int i = 0; i < devDesc.bNumConfigs; i++)
	{
		if(_pArrConfigDesc[ i ].bConfigurationValue == bConfigValue)
		{
			_iConfigIndex = i ;
			break ;
		}
	}
	
  GetDeviceStringDesc(_manufacturer, _deviceDesc.indexManufacturer);
  GetDeviceStringDesc(_product, _deviceDesc.indexProduct);
  GetDeviceStringDesc(_serialNum, _deviceDesc.indexSerialNum);
	USBDataHandler_DisplayDeviceStringDetails(this) ;
}

void UHCIDevice::GetDeviceStringDesc(upan::string& desc, int descIndex)
{
  desc = "Unknown";
  if(_usLangID == 0)
    return;

	unsigned short usDescValue = (0x3 << 8);
	char szPart[ 8 ];

  if(UHCIController_GetDescriptor(uiIOBase, _devAddr, usDescValue | descIndex, _usLangID, -1, szPart) != UHCIController_SUCCESS)
    return;

  int iLen = ((USBStringDescZero*)&szPart)->bLength;
  printf("\n String Index: %u, String Desc Size: %d", descIndex, iLen);
  if(iLen == 0)
    return;

  char* strDesc = new char[iLen];
  if(UHCIController_GetDescriptor(uiIOBase, _devAddr, usDescValue | descIndex, _usLangID, iLen, strDesc) != UHCIController_SUCCESS)
  {
    delete[] strDesc;
    return;
  }

  int j, k;
  for(j = 0, k = 0; j < (iLen - 2); k++, j += 2)
  {
    //TODO: Ignoring Unicode 2nd byte for the time.
    strDesc[k] = strDesc[j + 2];
  }
  strDesc[k] = '\0';

  desc = strDesc;
  delete[] strDesc;
}
