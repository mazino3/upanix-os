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
#ifndef _UHCI_DATA_HANDLER_H_
#define _UHCI_DATA_HANDLER_H_

#define UHCIDataHandler_SUCCESS		0
#define UHCIDataHandler_FRMQ_EMPTY	1
#define UHCIDataHandler_FRMQ_FULL	2
#define UHCIDataHandler_NO_FREE_FRM	3
#define UHCIDataHandler_FAILURE		4

#include <Global.h>

#define UHCI_DESC_ADDR(addr) ( addr & ~(0xF) )

typedef enum
{
	// QH Link Pointer
	UHCI_ATTR_QH_TO_TD_HEAD_LINK,
	UHCI_ATTR_QH_TO_QH_HEAD_LINK,
	UHCI_ATTR_QH_TO_TD_ELEM_LINK,
	UHCI_ATTR_QH_TO_QH_ELEM_LINK,
	UHCI_ATTR_QH_HEAD_LINK_TERMINATE,
	UHCI_ATTR_QH_ELEM_LINK_TERMINATE,

	// TD Link Pointer
	UHCI_ATTR_TD_VERTICAL_LINK,
	UHCI_ATTR_TD_HORIZONTAL_LINK,
	UHCI_ATTR_TD_QH_LINK,
	UHCI_ATTR_TD_TERMINATE,

	// TD Control And Status
	UHCI_ATTR_TD_CONTROL_SPD,
	UHCI_ATTR_TD_CONTROL_ERR_LEVEL,
	UHCI_ATTR_TD_CONTROL_LSD,
	UHCI_ATTR_TD_CONTROL_IOS,
	UHCI_ATTR_TD_CONTROL_IOC,
	UHCI_ATTR_TD_STATUS_ACTIVE,
	UHCI_ATTR_TD_RESET_LEN,
	UHCI_ATTR_TD_ACT_LEN,

	// TD Token - Address
	UHCI_ATTR_TD_MAXLEN,
	UHCI_ATTR_TD_DATA_TOGGLE,
	UHCI_ATTR_TD_ENDPT_ADDR,
	UHCI_ATTR_TD_DEVICE_ADDR,
	UHCI_ATTR_TD_PID,

	UHCI_ATTR_CTLSTAT_FULL,
	UHCI_ATTR_BUF_PTR,

} UHCIDescAttrType ;

typedef void FuncCopyDesc(void*, const void*, int) ;

UHCITransferDesc* UHCIDataHandler_CreateTransferDesc() ;
void UHCIDataHandler_ClearTransferDesc(UHCITransferDesc* pTD) ;
UHCIQueueHead* UHCIDataHandler_CreateQueueHead() ;
unsigned UHCIDataHandler_CreateFrameList() ;
void UHCIDataHandler_SetTDControl(UHCITransferDesc* pTD, byte bShortPacket, byte bErrLevel, byte bLowSpeed, 
								  byte bIOS, byte IOC, byte bActive, byte bResetActLen) ;
void UHCIDataHandler_SetTDToken(UHCITransferDesc* pTD, unsigned short usMaxLen, byte bSetToggle, 
		byte bEndPt, byte bDeviceAddr, byte bPID) ;
void UHCIDataHandler_SetQHAttribute(UHCIQueueHead* pQH, UHCIDescAttrType bAttrType, unsigned uiValue) ;
void UHCIDataHandler_SetTDAttribute(UHCITransferDesc* pTD, UHCIDescAttrType bAttrType, unsigned uiValue) ;
byte UHCIDataHandler_IsQHHeadLinkTerminated(const UHCIQueueHead* pQH) ;
byte UHCIDataHandler_IsQHElemLinkTerminated(const UHCIQueueHead* pQH) ;
void UHCIDataHandler_GetQHHeadLink(const UHCIQueueHead* pQH, UHCIDescAttrType* bAttrType, unsigned* uiValue) ;
void UHCIDataHandler_GetQHElemLink(const UHCIQueueHead* pQH, UHCIDescAttrType* bAttrType, unsigned* uiValue) ;
byte UHCIDataHandler_IsTDLinkTerminated(const UHCITransferDesc* pTD) ;
unsigned UHCIDataHandler_GetTDAttribute(UHCITransferDesc* pTD, UHCIDescAttrType bAttrType) ;

byte UHCIDataHandler_StartFrameCleaner() ;
byte UHCIDataHandler_CleanFrame(unsigned uiFrameNumber) ;

unsigned UHCIDataHandler_GetFrameListAddress() ;
unsigned UHCIDataHandler_GetFrameListEntry(unsigned uiFrameNumber) ;
byte UHCIDataHandler_GetNextFreeFrame(unsigned* uiFreeFrameID) ;
void UHCIDataHandler_SetFrameListEntry(unsigned uiFrameNumber, unsigned uiValue, bool bCleanBuffer, bool bCleanDescs) ;

#endif
