/*
 *	Mother Operating System - An x86 based Operating System
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
# include <RTC.h>
# include <PIC.h>
# include <PortCom.h>
# include <Display.h>
# include <AsmUtil.h>
# include <MemConstants.h>
# include <MemUtil.h>

#define RTC_COMMAND_PORT		0x70
#define RTC_DATA_PORT			0x71
#define RTC_RGSTR_SECOND		0x00
#define RTC_RGSTR_MINUTE		0x02
#define RTC_RGSTR_HOUR			0x04
#define RTC_RGSTR_DAYOFWEEK		0x06
#define RTC_RGSTR_DAYOFMONTH	0x07
#define RTC_RGSTR_MONTH			0x08
#define RTC_RGSTR_YEAR			0x09
#define RTC_RGSTR_CENTURY		0x32
#define RTC_RGSTR_STATUSA		0x0A
#define RTC_RGSTR_STATUSB		0x0B
#define RTC_RGSTR_STATUSC		0x0C

void RTC_Handler() ;

static const IRQ* RTC_pIRQ ;

bool RTC::Initialize()
{
	byte bStatusA, bStatusB, bStatusC ;
		
    /* Read Status A */
	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_STATUSA) ;
	bStatusA = PortCom_ReceiveByte(RTC_DATA_PORT) ;

    /* Read Status B */
	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_STATUSB) ;
	bStatusB = PortCom_ReceiveByte(RTC_DATA_PORT) ;

    /* Read Status C */
	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_STATUSC) ;
	bStatusC = PortCom_ReceiveByte(RTC_DATA_PORT) ;

    /* Enable Periodic Interrupt */
	bStatusB |= (1 << 6) ;

    /* Write Status B */
	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_STATUSB) ;
	PortCom_SendByte(RTC_DATA_PORT, bStatusB) ;

    /* Write Status C */
	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_STATUSC) ;
	PortCom_SendByte(RTC_DATA_PORT, bStatusC) ;

    RTC_pIRQ = PIC::RegisterIRQ(PIC::RTC_IRQ, (unsigned)&RTC_Handler) ;
	if(!RTC_pIRQ)
		return false ;
//	PIC::EnableInterrupt(PIC::RTC_IRQ) ;
	
    return true ;
}

void RTC::GetDateTime(RTCDateTime& rRTCDateTime)
{
	/* Read Date */
	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_DAYOFWEEK) ;
	rRTCDateTime.bDayOfWeek = PortCom_ReceiveByte(RTC_DATA_PORT) ;

	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_DAYOFMONTH) ;
	rRTCDateTime.bDayOfMonth = BCD_TO_DECIMAL(PortCom_ReceiveByte(RTC_DATA_PORT)) ;

	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_MONTH) ;
	rRTCDateTime.bMonth = BCD_TO_DECIMAL(PortCom_ReceiveByte(RTC_DATA_PORT)) ;

	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_YEAR) ;
	rRTCDateTime.bYear = BCD_TO_DECIMAL(PortCom_ReceiveByte(RTC_DATA_PORT)) ;

	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_CENTURY) ;
	rRTCDateTime.bCentury = BCD_TO_DECIMAL(PortCom_ReceiveByte(RTC_DATA_PORT)) ;

	/* Read Time */
	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_SECOND) ;
	rRTCDateTime.bSecond = BCD_TO_DECIMAL(PortCom_ReceiveByte(RTC_DATA_PORT)) ;

	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_MINUTE) ;
	rRTCDateTime.bMinute = BCD_TO_DECIMAL(PortCom_ReceiveByte(RTC_DATA_PORT)) ;

	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_HOUR) ;
	rRTCDateTime.bHour = BCD_TO_DECIMAL(PortCom_ReceiveByte(RTC_DATA_PORT)) ;
}

void RTC_Handler()
{
	unsigned GPRStack[NO_OF_GPR] ;
	AsmUtil_STORE_GPR(GPRStack) ;
	// Not required as DataSegment is not being accessed. But lets play it safe...
	AsmUtil_SET_KERNEL_DATA_SEGMENTS

	PortCom_SendByte(RTC_COMMAND_PORT, RTC_RGSTR_STATUSC) ;
	byte bUnused = PortCom_ReceiveByte(RTC_DATA_PORT) ;
	
	PIC::SendEOI(RTC_pIRQ) ;

	AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
	AsmUtil_RESTORE_GPR(GPRStack) ;

	asm("leave") ;
	asm("IRET") ;
}

