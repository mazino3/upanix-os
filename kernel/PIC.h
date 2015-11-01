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
#ifndef _PIC_H_
#define _PIC_H_

#include <Global.h>
#include <List.h>

class PIC;

class IRQ
{
	public:
		int GetIRQNo() const { return m_iIRQNo; }
		bool operator==(const IRQ& r) const { return m_iIRQNo == r.GetIRQNo() ; }

	private:
		explicit IRQ(int iIRQNo) : m_iIRQNo(iIRQNo) { }

	private:
		int m_iIRQNo;

	friend class PIC;
};

class PIC
{
	private:
		PIC();
	public:
		static PIC& Instance()
		{
			static PIC instance;
			return instance;
		}

		//Some standard IRQs
		const IRQ NO_IRQ;
		const IRQ TIMER_IRQ;
		const IRQ KEYBOARD_IRQ;
		const IRQ FLOPPY_IRQ;
		const IRQ RTC_IRQ;
		const IRQ MOUSE_IRQ;

		unsigned short m_IRQMask;

		void EnableAllInterrupts();
		void DisableAllInterrupts();
		void SendEOI(const IRQ& irq);
		void EnableInterrupt(const IRQ& irq);
		void DisableInterrupt(const IRQ& irq);
		const IRQ* RegisterIRQ(const int& iIRQNo, unsigned pHandler);
		bool RegisterIRQ(const IRQ& irq, unsigned pHandler);
		const IRQ* GetIRQ(const IRQ& irq);
		const IRQ* GetIRQ(const int& iIRQNo);

		void DisplayIRQList();

	public:
		static const int MAX_INTERRUPT = 24;
		static const int SLAVE_IRQNO_START = 8;

	private:
		static const unsigned short MASTER_PORTA = 0x20;
		static const unsigned short MASTER_PORTB = 0x21;
		static const unsigned short SLAVE_PORTA = 0xA0;
		static const unsigned short SLAVE_PORTB = 0xA1;

		static const unsigned short MASTER_IRQ_BASE = 0x20;
		static const unsigned short SLAVE_IRQ_BASE = 0x28;

		List<const IRQ*> _irqs;
} ;

#endif
