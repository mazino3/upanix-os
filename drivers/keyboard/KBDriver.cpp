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
#include <KBDriver.h>
#include <Display.h>
#include <DSUtil.h>
#include <PIC.h>
#include <IDT.h>
#include <AsmUtil.h>
#include <PortCom.h>
#include <ProcessManager.h>
#include <KBInputHandler.h>
#include <MemUtil.h>

#define KB_STAT_OBF 0x01
#define KB_STAT_IBF 0x02
#define KB_REBOOT	0xFE

DSUtil_Queue KBDriver_QueueBuffer ;

#define Keyboard_QUEUE_BUFFER_SIZE 100
static unsigned Keyboard_Buffer[Keyboard_QUEUE_BUFFER_SIZE] ;

void KBDriver_Initialize()
{
	DSUtil_InitializeQueue(&KBDriver_QueueBuffer, (unsigned)&Keyboard_Buffer, Keyboard_QUEUE_BUFFER_SIZE) ;

	PIC::Instance().DisableInterrupt(PIC::Instance().KEYBOARD_IRQ) ;
	PIC::Instance().RegisterIRQ(PIC::Instance().KEYBOARD_IRQ, (unsigned)&KBDriver_Handler) ;
	PIC::Instance().EnableInterrupt(PIC::Instance().KEYBOARD_IRQ) ;

	PortCom_ReceiveByte(KB_DATA_PORT) ;	

	KC::MDisplay().LoadMessage("Keyboard Initialization", Success) ;
}

void KBDriver_Handler()
{
	unsigned GPRStack[NO_OF_GPR] ;
	AsmUtil_STORE_GPR(GPRStack) ;
	AsmUtil_SET_KERNEL_DATA_SEGMENTS

	__volatile__ char key ;

	//KC::MDisplay().Message("\n KB IRQ") ;
	KBDriver_WaitForRead() ;
	if(!(PortCom_ReceiveByte(KB_STAT_PORT) & 0x20))
	{
		key = ((PortCom_ReceiveByte(KB_DATA_PORT)) & 0xFF) ;

		if(!KBInputHandler_Process(key))
		{
			KBDriver_PutToQueueBuffer((key & 0xFF)) ;
			ProcessManager_SignalInterruptOccured(PIC::Instance().KEYBOARD_IRQ);
		}
	}

	PIC::Instance().SendEOI(PIC::Instance().KEYBOARD_IRQ);

	AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
	AsmUtil_RESTORE_GPR(GPRStack) ;

	__asm__ __volatile__("LEAVE") ;
	__asm__ __volatile__("IRET") ;
}

byte KBDriver_GetCharInBlockMode(byte *data)
{		
	while(KBDriver_GetFromQueueBuffer(data) == KBDriver_ERR_BUFFER_EMPTY) 
	{
//		unsigned i ; for(i = 0; i < 999; i++) asm("nop") ;
//		ProcessManager_Sleep(10) ;
		ProcessManager_WaitOnInterrupt(PIC::Instance().KEYBOARD_IRQ);
	}

	return KBDriver_SUCCESS ;
}

byte KBDriver_GetCharInNonBlockMode(byte *data)
{
	return KBDriver_GetFromQueueBuffer(data) ;
}

byte KBDriver_GetFromQueueBuffer(byte *data)
{
	unsigned uiData ;

	if(DSUtil_ReadFromQueue(&KBDriver_QueueBuffer, &uiData) == DSUtil_ERR_QUEUE_EMPTY)
		return  KBDriver_ERR_BUFFER_EMPTY ;

	*data = (byte)(uiData) ;

	return KBDriver_SUCCESS ;
}

byte KBDriver_PutToQueueBuffer(byte data)
{
	unsigned uiData = (unsigned)data ;

	if(DSUtil_WriteToQueue(&KBDriver_QueueBuffer, uiData) == DSUtil_ERR_QUEUE_FULL)
		return KBDriver_ERR_BUFFER_FULL ;
	
	return KBDriver_SUCCESS ;
}

byte KBDriver_WaitForWrite()
{
	int iTimeOut = 0xFFFFF ;

	while((iTimeOut > 0) && (PortCom_ReceiveByte(KB_STAT_PORT) & KB_STAT_IBF))
		iTimeOut-- ;

	return (iTimeOut > 0) ;
}

byte KBDriver_WaitForRead()
{
	int iTimeOut = 0xFFFFF ;

	while((iTimeOut > 0) && !(PortCom_ReceiveByte(KB_STAT_PORT) & KB_STAT_OBF))
		iTimeOut-- ;

	return (iTimeOut > 0) ;
}

void KBDriver_Reboot()
{
	KBDriver_WaitForWrite() ;

	PortCom_SendByte(KB_STAT_PORT, KB_REBOOT) ;

	__asm__ __volatile__("cli; hlt") ;
}
