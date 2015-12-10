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
#include <KBDriver.h>
#include <Display.h>
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

static void KBDriver_Handler()
{
	unsigned GPRStack[NO_OF_GPR] ;
	AsmUtil_STORE_GPR(GPRStack) ;
	AsmUtil_SET_KERNEL_DATA_SEGMENTS

	__volatile__ char key ;

	//KC::MDisplay().Message("\n KB IRQ") ;
	KBDriver::Instance().WaitForRead();
	if(!(PortCom_ReceiveByte(KB_STAT_PORT) & 0x20))
	{
		key = ((PortCom_ReceiveByte(KB_DATA_PORT)) & 0xFF) ;

		if(!KBInputHandler_Process(key))
		{
			KBDriver::Instance().PutToQueueBuffer((key & 0xFF)) ;
      PIC::Instance().KEYBOARD_IRQ.Signal();
		}
	}

	PIC::Instance().SendEOI(PIC::Instance().KEYBOARD_IRQ);

	AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
	AsmUtil_RESTORE_GPR(GPRStack) ;

	__asm__ __volatile__("LEAVE") ;
	__asm__ __volatile__("IRET") ;
}

KBDriver::KBDriver() : _qBuffer(1024)
{
	PIC::Instance().DisableInterrupt(PIC::Instance().KEYBOARD_IRQ) ;
	PIC::Instance().RegisterIRQ(PIC::Instance().KEYBOARD_IRQ, (unsigned)&KBDriver_Handler) ;
	PIC::Instance().EnableInterrupt(PIC::Instance().KEYBOARD_IRQ) ;

	PortCom_ReceiveByte(KB_DATA_PORT) ;	

	KC::MDisplay().LoadMessage("Keyboard Initialization", Success) ;
}

bool KBDriver::GetCharInBlockMode(byte *data)
{		
	while(!GetFromQueueBuffer(data))
		ProcessManager::Instance().WaitOnInterrupt(PIC::Instance().KEYBOARD_IRQ);
	return true;
}

bool KBDriver::GetCharInNonBlockMode(byte *data)
{
	return GetFromQueueBuffer(data) ;
}

bool KBDriver::GetFromQueueBuffer(byte *data)
{
  if(_qBuffer.empty())
    return false;
  *data = _qBuffer.front();
  _qBuffer.pop_front();
  return true;
}

bool KBDriver::PutToQueueBuffer(byte data)
{
  return _qBuffer.push_back(data);
}

bool KBDriver::WaitForWrite()
{
	int iTimeOut = 0xFFFFF ;
	while((iTimeOut > 0) && (PortCom_ReceiveByte(KB_STAT_PORT) & KB_STAT_IBF))
		iTimeOut--;
	return (iTimeOut > 0) ;
}

bool KBDriver::WaitForRead()
{
	int iTimeOut = 0xFFFFF ;
	while((iTimeOut > 0) && !(PortCom_ReceiveByte(KB_STAT_PORT) & KB_STAT_OBF))
		iTimeOut--;
	return (iTimeOut > 0) ;
}

void KBDriver::Reboot()
{
	WaitForWrite();
	PortCom_SendByte(KB_STAT_PORT, KB_REBOOT);
	__asm__ __volatile__("cli; hlt");
}
