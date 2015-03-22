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

#include <PIC.h>
#include <PortCom.h>
#include <Display.h>
#include <AsmUtil.h>
#include <IDT.h>
#include <MemConstants.h>
#include <stdio.h>
#include <Alloc.h>

unsigned short PIC::m_IRQMask ;
// Actual constuction is done using in place new operator in Initialize function below
const IRQ PIC::NO_IRQ(999) ;

const int PIC::TIMER_IRQ(0) ;
const int PIC::KEYBOARD_IRQ(1) ;
const int PIC::FLOPPY_IRQ(6) ;
const int PIC::RTC_IRQ(PIC::SLAVE_IRQNO_START) ;
const int PIC::MOUSE_IRQ(12) ;

void PIC::Initialize()
{
	static bool bDone = false ;
	if(bDone)
	{
		KC::MDisplay().Message("\n PIC is already initialized!") ;
		return ;
	}

	bDone = true ;
	new ((void*)&NO_IRQ)IRQ(999) ;
	// Sending ICW1 to master and slave PIC 
	PortCom_SendByte(MASTER_PORTA, 0x11) ;
	PortCom_SendByte(SLAVE_PORTA, 0x11) ;

	// Sending base vector number of master and slave PIC 
	PortCom_SendByte(MASTER_PORTB, MASTER_IRQ_BASE) ; // base vector number of master PIC
	PortCom_SendByte(SLAVE_PORTB, SLAVE_IRQ_BASE) ; // base vector number of slave PIC

	// Sending connection parameters between the PICs to the PICs 
	PortCom_SendByte(MASTER_PORTB, 0x04) ; // IRQ2 is connected to slave PIC
	PortCom_SendByte(SLAVE_PORTB, 0x02) ; // IRQ2 of master PIC is used for the slave PIC

	// Intel environment, manual EOI
	PortCom_SendByte(MASTER_PORTB, 0x01) ;
	PortCom_SendByte(SLAVE_PORTB, 0x01) ;

	// Disable All Interrupts
	PortCom_SendByte(MASTER_PORTB, 0xff) ;
	PortCom_SendByte(SLAVE_PORTB, 0xff) ;

	m_IRQMask = 0xFFFF ;

	KC::MDisplay().LoadMessage("PIC Initialization", SUCCESS) ;
}

void PIC::EnableAllInterrupts()
{
	__asm__ __volatile__("sti") ;
}

void PIC::DisableAllInterrupts()
{
	__asm__ __volatile__("cli") ;
}

bool PIC::SendEOI(const IRQ* pIRQ)
{
	if(!pIRQ)
	{
		KC::MDisplay().Number("\n IRQ is not registered with PIC! = ", pIRQ->GetIRQNo()) ;
		return false ;
	}

	PortCom_SendByte(MASTER_PORTA, 0x20) ;
	if(pIRQ->GetIRQNo() >= SLAVE_IRQNO_START)
		PortCom_SendByte(SLAVE_PORTA, 0x20) ;

	return true ;
}

void PIC::EnableInterrupt(const int& iIRQNo)
{
	__volatile__ unsigned uiIntFlag ;
	SAFE_INT_DISABLE(uiIntFlag) ;

	m_IRQMask &= ~(1 << iIRQNo) ;
	if(iIRQNo >= SLAVE_IRQNO_START)
		m_IRQMask &= ~(1 << 2) ;
	
	PortCom_SendByte(MASTER_PORTB, (m_IRQMask & 0xFF)) ;
	PortCom_SendByte(SLAVE_PORTB, ((m_IRQMask >> 8) & 0xFF)) ;

	SAFE_INT_ENABLE(uiIntFlag) ;
}

void PIC::DisableInterrupt(const int& iIRQNo)
{
	__volatile__ unsigned uiIntFlag ;
	SAFE_INT_DISABLE(uiIntFlag) ;

	m_IRQMask |= (1 << iIRQNo) ;
	if((m_IRQMask & 0xFF00) == 0xFF00) // All Slave Ints disabled
		m_IRQMask |= (1 << 2) ;

	PortCom_SendByte(MASTER_PORTB, (m_IRQMask & 0xFF)) ;
	PortCom_SendByte(SLAVE_PORTB, ((m_IRQMask >> 8) & 0xFF)) ;

	SAFE_INT_ENABLE(uiIntFlag) ;
}

const IRQ* PIC::RegisterIRQ(const int& iIRQNo, unsigned pHandler)
{
	if(iIRQNo < 0 && iIRQNo >= MAX_INTERRUPT)
		return NULL ;

	if(GetIRQ(iIRQNo))
	{
		KC::MDisplay().Number("\n IRQ is already registered! = ", iIRQNo) ;
		return NULL ;
	}

	IRQ* pIRQ = new IRQ(iIRQNo) ;
	GetIRQList().PushBack(pIRQ) ;

	unsigned uiIRQBase = MASTER_IRQ_BASE + iIRQNo ;

	if(iIRQNo >= SLAVE_IRQNO_START)
		uiIRQBase = SLAVE_IRQ_BASE + iIRQNo - SLAVE_IRQNO_START ;

	IDT::LoadEntry(uiIRQBase, pHandler, SYS_CODE_SELECTOR, 0x8E) ;

	return pIRQ ;
}

const IRQ* PIC::GetIRQ(const int& iIRQNo)
{
	IRQ* pIRQ ;

	GetIRQList().Reset() ;
	for(int i = 0; GetIRQList().GetNext(); i++)
	{
		GetIRQList().GetCurrentValue(pIRQ) ;
		if(pIRQ->GetIRQNo() == iIRQNo)
			return pIRQ ;
	}

	return NULL ;
}

List<IRQ*>& PIC::GetIRQList()
{
	static List<IRQ*> m_mIRQList ;
	return m_mIRQList ;
}

void PIC::DisplayIRQList()
{
	IRQ* pIRQ ;
	GetIRQList().Reset() ;
	for(int i = 0; GetIRQList().GetNext(); i++)
	{
		GetIRQList().GetCurrentValue(pIRQ) ;
		KC::MDisplay().Number("\n IRQ = ", pIRQ->GetIRQNo()) ;
	}
}
