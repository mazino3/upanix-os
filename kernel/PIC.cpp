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

#include <PIC.h>
#include <PortCom.h>
#include <Display.h>
#include <AsmUtil.h>
#include <IDT.h>
#include <MemConstants.h>
#include <stdio.h>
#include <Alloc.h>
#include <Atomic.h>

void IRQ::Signal() const
{
	Atomic::Swap(_interruptOn, 1);
  if(_qInterrupt.size() < MAX_PROC_ON_INT_QUEUE)
    _qInterrupt.push_back(1);
}

int IRQ::InterruptOn() const
{
	return Atomic::Swap(_interruptOn, 0);
}

bool IRQ::Consume() const
{
  PICGuard g(*this);
  InterruptOn();
  if(_qInterrupt.empty())
    return false;
  _qInterrupt.pop_front();
  return true;
}

PIC::PIC() : NO_IRQ(999),
	TIMER_IRQ(0),
	KEYBOARD_IRQ(1),
	FLOPPY_IRQ(6),
	RTC_IRQ(PIC::SLAVE_IRQNO_START),
	MOUSE_IRQ(12)
{
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

	KC::MDisplay().LoadMessage("PIC Initialization", Success) ;
}

void PIC::EnableAllInterrupts()
{
	__asm__ __volatile__("sti") ;
}

void PIC::DisableAllInterrupts()
{
	__asm__ __volatile__("cli") ;
}

void PIC::SendEOI(const IRQ& irq)
{
	PortCom_SendByte(MASTER_PORTA, 0x20) ;
	if(irq.GetIRQNo() >= SLAVE_IRQNO_START)
		PortCom_SendByte(SLAVE_PORTA, 0x20) ;
}

void PIC::EnableInterrupt(const IRQ& irq)
{
	__volatile__ unsigned uiIntFlag ;
	SAFE_INT_DISABLE(uiIntFlag) ;

	int iIRQNo = irq.GetIRQNo();
	m_IRQMask &= ~(1 << iIRQNo) ;
	if(iIRQNo >= SLAVE_IRQNO_START)
		m_IRQMask &= ~(1 << 2) ;
	
	PortCom_SendByte(MASTER_PORTB, (m_IRQMask & 0xFF)) ;
	PortCom_SendByte(SLAVE_PORTB, ((m_IRQMask >> 8) & 0xFF)) ;

	SAFE_INT_ENABLE(uiIntFlag) ;
}

void PIC::DisableInterrupt(const IRQ& irq)
{
	__volatile__ unsigned uiIntFlag ;
	SAFE_INT_DISABLE(uiIntFlag) ;

	int iIRQNo = irq.GetIRQNo();
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
		return NULL;

	IRQ* pIRQ = new IRQ(iIRQNo);
	if(!RegisterIRQ(*pIRQ, pHandler))
	{
		delete pIRQ;
		pIRQ = NULL;
	}
	return pIRQ;
}

bool PIC::RegisterIRQ(const IRQ& irq, unsigned pHandler)
{
	if(GetIRQ(irq))
	{
		printf("\nIRQ '%d' is already registered!", irq.GetIRQNo());
		return false;
	}

	_irqs.push_back(&irq);

	int iIRQNo = irq.GetIRQNo();
	unsigned uiIRQBase = MASTER_IRQ_BASE + iIRQNo ;

	if(iIRQNo >= SLAVE_IRQNO_START)
		uiIRQBase = SLAVE_IRQ_BASE + iIRQNo - SLAVE_IRQNO_START ;

	IDT::Instance().LoadEntry(uiIRQBase, pHandler, SYS_CODE_SELECTOR, 0x8E) ;

	return true;
}

const IRQ* PIC::GetIRQ(const IRQ& irq)
{
	return GetIRQ(irq.GetIRQNo());
}

const IRQ* PIC::GetIRQ(const int& iIRQNo)
{
	for(auto i : _irqs)
    if(i->GetIRQNo() == iIRQNo)
      return i;
  return NULL;
}

void PIC::DisplayIRQList()
{
	for(auto i : _irqs)
		printf("\n IRQ = %d", i->GetIRQNo()) ;
}
