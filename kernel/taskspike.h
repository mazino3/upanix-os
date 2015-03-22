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

#include <Process.h>

void EFlags() ;
void DoContextSwitch() ;
void DoContextSwitchTaskGate1() ;
void DoContextSwitchTaskGate2() ;
void KC::MDisplay().Task1() ;
void KC::MDisplay().Task2() ;
void TaskSpike() ;

void
EFlags()
{
	unsigned uiFlag ;
	
	asm ("pushf") ;
	asm ("pop %eax") ;
	asm ("pop %eax") ;
	asm ("movw %%ax, %0" : "=g"(uiFlag) :) ;

	KC::MDisplay().Address("\n\nFLAG = ", uiFlag) ;
}

void KC::MDisplay().Task1()
{
	unsigned i, j, k = 0 ;
	while(k < 20)
	{
		for(i = 0; i < 1000; i++)
			for(j = 0; j < 1000; j++) 
				asm("nop") ;
				
//		KC::MDisplay().Message("\n Task Switch Thru Task Gate 1\n", ' ') ;
		KC::MDisplay().Character('1', ' ') ;
		k++ ;
	//	asm("int $0x09") ;
	}
	asm("IRET") ;
}

void KC::MDisplay().Task2()
{
	unsigned i, j, k = 0 ;
	while(k < 3)
	{
		for(i = 0; i < 1000; i++)
			for(j = 0; j < 1000; j++) 
				asm("nop") ;
				
		KC::MDisplay().Message("\n Task Switch Thru Task Gate 2 With Sleep\n", ' ') ;
		Process_Sleep(2) ;
		k++ ;
	}
	asm("IRET") ;
}

void DoContextSwitch()
{
	byte bStatus ;

	bStatus = Floppy_Read(0, MemUtil_GetDS(), 39, 40, 512, (byte*)0x300000) ;

	if(bStatus == Floppy_SUCCESS)
	{
		KC::MDisplay().Message("\nFLOPPY READ SUCCESS\n", KC::MDisplay().WHITE_ON_BLACK()) ;

		byte buf[30] ;
		unsigned i ;
		MemUtil_CopyMemory(SYS_LINEAR_SELECTOR, G_uiPhysicalFloppyDMABufferAddress, MemUtil_GetDS(), (unsigned)&buf, 30) ;

		for(i = 0; i < 30; i++)
			KC::MDisplay().Address("", buf[i]) ;
	}
	else
	{
		KC::MDisplay().Address("\nFLOPPY READ FAILURE :", bStatus) ;
		return ;
	}

	Process_BuildTaskState(0x300000, 0x80a00 + 104, 0,0x300300) ;
	/*asm("movw $0x0, (0x1000)") ;
	asm("movw $0x28, (0x1004)") ;
	asm("lcall *(%0)" : : "r"(0x1000)) ;*/
	
	asm("lcall $0x30, $0x00"); 
}

void DoContextSwitchTaskGate1()
{
	Process_BuildTaskState((unsigned)&KC::MDisplay().Task1, 0x80a00 + 104, 2,0x300100) ;
	asm("lcall $0x30,$0x00") ;
}

void
TaskSpike()
{
//	IDT_LoadEntry(0x09, (unsigned int)&testLoop, SYS_CODE_SELECTOR, 0x8E) ;
	
//	Process_BuildTaskState((unsigned)&KC::MDisplay().Task1, 0x80a00 + 104, 2, 0x300100) ;
//	Process_BuildTaskState((unsigned)&KC::MDisplay().Task2, 0x80a00 + 104 + 104, 2, 0x300300) ;
	
//	Process_BuildTaskState((unsigned)&Console_StartMOSConsole, 0x80a00 + 104 + 104, 2, 0x300300) ;
	
//	DoContextSwitch() ;
//	DoContextSwitchTaskGate() ;

	//Process_Create((unsigned)&KC::MDisplay().Task1, 12 * 1024 * 1024 + 200) ;
	//Process_Create((unsigned)&KC::MDisplay().Task2, 12 * 1024 * 1024 + 400) ;
	Process_CreateKernelImage((unsigned)&KC::MDisplay().Task1) ;
	Process_CreateKernelImage((unsigned)&KC::MDisplay().Task1) ;
	Process_StartScheduler() ;
}
