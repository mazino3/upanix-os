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

/* Intel 8253 Programmable Interval Timer */

#include <PIT.h>
#include <PIC.h>
#include <IDT.h>
#include <AsmUtil.h>
#include <PortCom.h>
#include <Display.h>
#include <MemConstants.h>
#include <MemUtil.h>
#include <Atomic.h>

#define CLOCK_TICK_RATE 1193181 // Input Frequency of PIT
#define INT_PER_SEC 100

#define PIT_MODE_PORT 0x43
#define PIT_COUNTER_0_PORT 0x40

__volatile__ static unsigned PIT_ClockCountForSleep ;
__volatile__ static unsigned char Process_bContextSwitch ;
__volatile__ static int Process_iTaskSwitch ;

static const IRQ* PIT_pIRQ ;

void PIT_Initialize()
{
	byte bStatus = SUCCESS ;
	PIT_ClockCountForSleep = 0 ;
	Process_bContextSwitch = false ;
	Process_iTaskSwitch = true ;

	__volatile__ unsigned uiIntFlag ;
	SAFE_INT_DISABLE(uiIntFlag) ;
	
	unsigned uiTimerRate = CLOCK_TICK_RATE / INT_PER_SEC ;

	PortCom_SendByte(PIT_MODE_PORT, 0x34) ;				// Set Timer to Mode 2 -- Free Running LSB/MSB
	PortCom_SendByte(PIT_COUNTER_0_PORT, uiTimerRate & 0xFF) ;			// Clock Divisor LSB
	PortCom_SendByte(PIT_COUNTER_0_PORT, (uiTimerRate >> 8) & 0xFF) ;	// Clock Divisor MSB

	KC::MDisplay().Number("\n TIMER_IRQ = ", PIC::TIMER_IRQ) ;
	PIT_pIRQ = PIC::RegisterIRQ(PIC::TIMER_IRQ, (unsigned)&PIT_Handler) ;
	if(!PIT_pIRQ)
		bStatus = FAILURE ;
	
	SAFE_INT_ENABLE(uiIntFlag) ;	

	KC::MDisplay().LoadMessage("Timer Initialization", bStatus ? Success : Failure);
}

__volatile__ const IRQ* PIT_GetIRQ() { return PIT_pIRQ ; }

__volatile__ unsigned PIT_GetClockCount() { return PIT_ClockCountForSleep ; }

__volatile__ unsigned char PIT_IsContextSwitch() { return Process_bContextSwitch ; }
__volatile__ void PIT_SetContextSwitch(bool flag) { Process_bContextSwitch = flag ; }

__volatile__ unsigned char PIT_IsTaskSwitch() { return Process_iTaskSwitch ; }
__volatile__ void PIT_SetTaskSwitch(bool flag) { Atomic::Swap(Process_iTaskSwitch, flag) ; }

void PIT_Handler()
{
	__volatile__ unsigned GPRStack[NO_OF_GPR] ;
	AsmUtil_STORE_GPR(GPRStack) ;
	
	__volatile__ unsigned short usDS = MemUtil_GetDS() ; 
	__volatile__ unsigned short usES = MemUtil_GetES() ; 
	__volatile__ unsigned short usFS = MemUtil_GetFS() ; 
	__volatile__ unsigned short usGS = MemUtil_GetGS() ;

	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
	__asm__ __volatile__("popw %ds") ; 
	__asm__ __volatile__("popw %fs") ; 
	__asm__ __volatile__("popw %gs") ; 

	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
	__asm__ __volatile__("popw %es") ; 

	// 1 Int --> 10ms
	PIT_ClockCountForSleep++ ;

	if(Process_iTaskSwitch == true)
	{
		__volatile__ unsigned uiTaskReg = 0;
		__asm__ __volatile__("STR %ax") ;
		__asm__ __volatile__("movw %%ax, %0" : "=m"(uiTaskReg) :) ;

		if(uiTaskReg == USER_TSS_SELECTOR) 
		{
			Process_bContextSwitch = true ;

			__asm__ __volatile__("pushf") ;
			__asm__ __volatile__("popl %eax") ;
			__asm__ __volatile__("mov $0x4000, %ebx") ;
			__asm__ __volatile__("or %ebx, %eax") ;
			__asm__ __volatile__("pushl %eax") ;
			__asm__ __volatile__("popf") ;
			
			PIC::SendEOI(PIT_pIRQ) ;

			__asm__ __volatile__("IRET") ;
		
			__asm__ __volatile__("pushf") ;
			__asm__ __volatile__("popl %eax") ;
			__asm__ __volatile__("mov $0xBFFF, %ebx") ;
			__asm__ __volatile__("and %ebx, %eax") ;
			__asm__ __volatile__("pushl %eax") ;
			__asm__ __volatile__("popf") ;

//			AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
			__asm__ __volatile__("movw %%ss:%0, %%ds" :: "m"(usDS) ) ;
			__asm__ __volatile__("movw %%ss:%0, %%es" :: "m"(usES) ) ;
			__asm__ __volatile__("movw %%ss:%0, %%fs" :: "m"(usFS) ) ;
			__asm__ __volatile__("movw %%ss:%0, %%gs" :: "m"(usGS) ) ;
	
			AsmUtil_RESTORE_GPR(GPRStack) ;

			__asm__ __volatile__("leave") ;
			__asm__ __volatile__("IRET") ;
		}
	}

	__asm__ __volatile__("movw %%ss:%0, %%ds" :: "m"(usDS) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%es" :: "m"(usES) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%fs" :: "m"(usFS) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%gs" :: "m"(usGS) ) ;

	PIC::SendEOI(PIT_pIRQ) ;

	AsmUtil_RESTORE_GPR(GPRStack) ;

	__asm__ __volatile__("leave") ;
	__asm__ __volatile__("IRET") ;
}

__volatile__ unsigned PIT_RoundSleepTime(__volatile__ unsigned uiSleepTime)
{
	if((uiSleepTime % 10) >= 5)
		return uiSleepTime / 10 + 1 ;

	return uiSleepTime / 10 ;
}

