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
#ifndef _EHCI_STRUCTURES_H_
#define _EHCI_STRUCTURES_H_

#include <Global.h>
#include <USBConstants.h>
#include <PCIBusHandler.h>
#include <list.h>

/* EHCI Interrupt Flags */
#define INTR_ASYNC_ADVANCE	0x20
#define INTR_HOST_SYS_ERR	0x10
#define INTR_FRM_LST_ROLL	0x08
#define INTR_PORT_CHG		0x04
#define INTR_USB_ERR		0x02
#define INTR_USB			0x01

/* EHCI Command */
#define ASYNC_SCHEDULE_ENABLE		0x20
#define PERIODIC_SCHEDULE_ENABLE	0x10
#define INTR_THRESHOLD_POS			16

#define MAX_EHCI_PORTS 8

typedef struct
{
	byte bCapLength ;
	byte bReserved ;
	unsigned short usHCIVersion ;
	unsigned uiHCSParams ;
	unsigned uiHCCParams ;
	byte bHCSPPortRoute[ 8 ] ;
} PACKED EHCICapRegisters ;

typedef struct
{
	unsigned uiUSBCommand ;
	unsigned uiUSBStatus ;
	unsigned uiUSBInterrupt ;
	unsigned uiFrameIndex ;
	unsigned uiCtrlDSSegment ;
	unsigned uiPeriodicListBase ;
	unsigned uiAsyncListBase ;
	unsigned uiReserved[ 9 ] ;
	unsigned uiConfigFlag ;
	unsigned uiPortReg[ MAX_EHCI_PORTS ] ;
} PACKED EHCIOpRegisters ;

typedef struct
{
	unsigned uiNextpTDPointer ;
	unsigned uiAltpTDPointer ;
	unsigned uipTDToken ;

	unsigned uiBufferPointer[ 5 ] ;
	unsigned uiExtendedBufferPointer[ 5 ] ;

} PACKED EHCIQTransferDesc ;

typedef struct
{

	unsigned uiHeadHorizontalLink ;

	/* Transfer Overlay */
	unsigned uiEndPointCap_Part1 ;
	unsigned uiEndPointCap_Part2 ;

	/* Transfer Overlay */
	unsigned uiCurrrentpTDPointer ;
	unsigned uiNextpTDPointer ;
		/* Transfer Results */
	unsigned uiAltpTDPointer_NAKCnt ;
	unsigned uipQHToken ;
	unsigned uiBufferPointer[ 5 ] ;

	unsigned uiExtendedBufferPointer[ 5 ] ;

} PACKED EHCIQueueHead ;

typedef struct
{
	EHCIQueueHead* pQH ;
	EHCIQTransferDesc* pTDStart ;

  upan::list<unsigned> dStorageList;
} EHCITransaction ;

typedef struct
{
	PCIEntry* pPCIEntry ;
	bool bSetupSuccess ;
	EHCICapRegisters* pCapRegs ;
	EHCIOpRegisters* pOpRegs ;

	EHCIQueueHead* pAsyncReclaimQueueHead ;
} EHCIController ;

typedef struct
{
	EHCIController* pController ;
	EHCIQueueHead* pControlQH ;
	EHCIQueueHead* pBulkInEndPt ;
	EHCIQueueHead* pBulkOutEndPt;
} EHCIDevice ;

#endif
