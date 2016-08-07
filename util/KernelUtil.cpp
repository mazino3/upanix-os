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
#include <PIT.h>
#include <IrqManager.h>
#include <ProcessManager.h>
#include <KernelUtil.h>

void KernelUtil::Wait(__volatile__ unsigned uiTimeInMilliSec)
{
	uiTimeInMilliSec = PIT_RoundSleepTime(uiTimeInMilliSec) ;
	__volatile__ unsigned uiStartTime = PIT_GetClockCount() ;

	IrqManager::Instance().EnableIRQ(IrqManager::Instance().TIMER_IRQ) ;
	while((PIT_GetClockCount() - uiStartTime) < uiTimeInMilliSec)
	{
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
	}
}

void KernelUtil::WaitOnInterrupt(const IRQ& irq)
{
	while(!irq.Consume())
	{	
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
	}
}

void KernelUtil::TightLoopWait(unsigned loop)
{
	unsigned i ;
	for(i = 0; i < loop * loop * loop; i++)
	{
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
	}
}

void KernelUtil::ScheduleTimedTask(const char* szName, unsigned uiTimeInMilliSec, unsigned CallBackFunction)
{
	int pid ;
	ProcessManager::Instance().CreateKernelImage((unsigned)&SystemTimer, NO_PROCESS_ID, false, uiTimeInMilliSec, CallBackFunction, &pid, szName) ;
}

void KernelUtil::SystemTimer(unsigned uiTimeInMilliSec, KernelUtilTimerFunc* CallBackFunction)
{
	do
	{
		ProcessManager::Instance().Sleep(uiTimeInMilliSec) ;
	} while(CallBackFunction()) ;
	ProcessManager_EXIT() ;
}

