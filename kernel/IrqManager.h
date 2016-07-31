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
#ifndef _IRQ_MANAGER_H_
#define _IRQ_MANAGER_H_

#include <Global.h>
#include <list.h>

class IrqManager;

class IRQ
{
	public:
		int GetIRQNo() const { return m_iIRQNo; }
		bool operator==(const IRQ& r) const { return m_iIRQNo == r.GetIRQNo() ; }
    void Signal() const;
    bool Consume() const;
    int InterruptOn() const;

	private:
		explicit IRQ(int iIRQNo) : m_iIRQNo(iIRQNo), _interruptOn(0) { }

	private:
    static const int MAX_PROC_ON_INT_QUEUE = 8 * 1024;
		const int m_iIRQNo;
    mutable int _interruptOn;
    mutable upan::list<bool> _qInterrupt;

	friend class IrqManager;
};

class IrqManager
{
  private:
    IrqManager(const IrqManager&);
    static IrqManager* _instance;

  protected:
    IrqManager();
    static const int IRQ_BASE = 32;
		static const int MAX_INTERRUPT = 24;

  public:
    static IrqManager& Instance();
    static void Initialize();
    static bool Exists() { return _instance != nullptr; }

    bool IsApic() const { return _isApic; }

		//Some standard IRQs
		const IRQ NO_IRQ;
		const IRQ TIMER_IRQ;
		const IRQ KEYBOARD_IRQ;
		const IRQ FLOPPY_IRQ;
		const IRQ RTC_IRQ;
		const IRQ MOUSE_IRQ;

		void EnableAllInterrupts();
		void DisableAllInterrupts();
		const IRQ* RegisterIRQ(const int& iIRQNo, unsigned pHandler);
		bool RegisterIRQ(const IRQ& irq, unsigned pHandler);
		const IRQ* GetIRQ(const IRQ& irq);
		const IRQ* GetIRQ(const int& iIRQNo);
		void DisplayIRQList();

		virtual void SendEOI(const IRQ&) = 0;
		virtual void EnableIRQ(const IRQ&) = 0;
		virtual void DisableIRQ(const IRQ&) = 0;

  private:
		upan::list<const IRQ*> _irqs;
    bool _isApic;
};

class IrqGuard
{
  public:
    IrqGuard(const IRQ& irq) : _irq(&irq)
    {
      IrqManager::Instance().DisableIRQ(*_irq);
    }
    IrqGuard() : _irq(nullptr)
    {
      if(!IrqManager::Exists())
        return;
//      __asm__ __volatile__("pushf");
  //    __asm__ __volatile__("popl %0" : "=m"(_allIntSyncFlag) : );
    //  if(_allIntSyncFlag & 0x0200)
        IrqManager::Instance().DisableAllInterrupts();
    }
    ~IrqGuard()
    {
      if(!IrqManager::Exists())
        return;
      if(_irq)
        IrqManager::Instance().EnableIRQ(*_irq);
      else
      {
//        if(_allIntSyncFlag & 0x0200)
          IrqManager::Instance().EnableAllInterrupts();
      }
    }
  private:
    const IRQ* _irq;
    uint32_t _allIntSyncFlag;
};

#endif
