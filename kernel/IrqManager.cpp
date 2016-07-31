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

#include <stdio.h>
#include <IrqManager.h>
#include <PIC.h>
#include <Apic.h>
#include <Atomic.h>
#include <IDT.h>

IrqManager* IrqManager::_instance = nullptr;

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
  IrqGuard g(*this);
  InterruptOn();
  if(_qInterrupt.empty())
    return false;
  _qInterrupt.pop_front();
  return true;
}

void IrqManager::Initialize()
{
  static PIC pic;
  if(Apic::IsAvailable())
  {
    pic.DisableForAPIC();
    static Apic apic;
    _instance = &apic;
    _instance->_isApic = true;
    apic.Initialize();
  }
  else
  {
    _instance = &pic;
    _instance->_isApic = false;
  }
}

IrqManager& IrqManager::Instance()
{
  if(!_instance)
    throw upan::exception(XLOC, "IRQ Manager is not initialized yet!");
  return *_instance;
}

IrqManager::IrqManager() : NO_IRQ(999),
	TIMER_IRQ(0),
	KEYBOARD_IRQ(1),
	FLOPPY_IRQ(6),
	RTC_IRQ(8),
	MOUSE_IRQ(12),
  _isApic(false)
{
}

const IRQ* IrqManager::RegisterIRQ(const int& iIRQNo, unsigned pHandler)
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

bool IrqManager::RegisterIRQ(const IRQ& irq, unsigned pHandler)
{
	if(GetIRQ(irq))
	{
		printf("\nIRQ '%d' is already registered!", irq.GetIRQNo());
		return false;
	}

	_irqs.push_back(&irq);

	IDT::Instance().LoadEntry(IRQ_BASE + irq.GetIRQNo(), pHandler, SYS_CODE_SELECTOR, 0x8E) ;

	return true;
}

const IRQ* IrqManager::GetIRQ(const IRQ& irq)
{
	return GetIRQ(irq.GetIRQNo());
}

const IRQ* IrqManager::GetIRQ(const int& iIRQNo)
{
	for(auto i : _irqs)
    if(i->GetIRQNo() == iIRQNo)
      return i;
  return NULL;
}

void IrqManager::DisplayIRQList()
{
	for(auto i : _irqs)
		printf("\n IRQ = %d", i->GetIRQNo()) ;
}
