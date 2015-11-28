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
#ifndef _ASMUTIL_H_
#define _ASMUTIL_H_

#define NO_OF_GPR 8

#include <MemUtil.h>
#include <MemConstants.h>

//unsigned KBDriver_GPRStack[NO_OF_GPR] ;
//unsigned PIT_GPRStack[NO_OF_GPR] ;
//unsigned Floppy_GPRStack[NO_OF_GPR] ;
//unsigned RTC_GPRStack[NO_OF_GPR] ;
//unsigned IDEPrimary_GPRStack[NO_OF_GPR] ;
//unsigned IDESecondary_GPRStack[NO_OF_GPR] ;

#define AsmUtil_STORE_GPR(BufferStack) \
__asm__ __volatile__("movl %%eax, %%ss:%0" : "=m"(BufferStack[0]): ) ; \
__asm__ __volatile__("movl %%ebx, %%ss:%0" : "=m"(BufferStack[1]): ) ; \
__asm__ __volatile__("movl %%ecx, %%ss:%0" : "=m"(BufferStack[2]): ) ; \
__asm__ __volatile__("movl %%edx, %%ss:%0" : "=m"(BufferStack[3]): ) ; \
__asm__ __volatile__("movl %%esi, %%ss:%0" : "=m"(BufferStack[4]): ) ; \
__asm__ __volatile__("movl %%edi, %%ss:%0" : "=m"(BufferStack[5]): ) ;
//__asm__ __volatile__("movl %%ebp, %%ss:%0" : "=m"(BufferStack[6]): ) ; 
//__asm__ __volatile__("movl %%esp, %%ss:%0" : "=m"(BufferStack[7]): )

#define AsmUtil_RESTORE_GPR(BufferStack) \
__asm__ __volatile__("movl %%ss:%0, %%eax" : : "m"(BufferStack[0])) ; \
__asm__ __volatile__("movl %%ss:%0, %%ebx" : : "m"(BufferStack[1])) ; \
__asm__ __volatile__("movl %%ss:%0, %%ecx" : : "m"(BufferStack[2])) ; \
__asm__ __volatile__("movl %%ss:%0, %%edx" : : "m"(BufferStack[3])) ; \
__asm__ __volatile__("movl %%ss:%0, %%esi" : : "m"(BufferStack[4])) ; \
__asm__ __volatile__("movl %%ss:%0, %%edi" : : "m"(BufferStack[5])) ;
//__asm__ __volatile__("movl %%ss:%0, %%ebp" : : "m"(BufferStack[6])) ; 
//__asm__ __volatile__("movl %%ss:%0, %%esp" : : "m"(BufferStack[7])) 

#define AsmUtil_LOAD_KERNEL_SEGS() \
{ \
__volatile__ unsigned uiIntFlag ; \
SAFE_INT_DISABLE(uiIntFlag) ; \
__asm__ __volatile__("movw %ds, %ss:(0)") ; \
__asm__ __volatile__("movw %fs, %ss:(2)") ; \
__asm__ __volatile__("movw %gs, %ss:(4)") ; \
__asm__ __volatile__("movw %es, %ss:(6)") ; \
\
__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; \
__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; \
__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; \
__asm__ __volatile__("popw %ds") ; \
__asm__ __volatile__("popw %fs") ; \
__asm__ __volatile__("popw %gs") ; \
\
__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; \
__asm__ __volatile__("popw %es") ; \
SAFE_INT_ENABLE(uiIntFlag) ; \
}

#define AsmUtil_UNLOAD_KERNEL_SEGS() \
{ \
/*__volatile__ unsigned uiIntFlag ;*/ \
/*SAFE_INT_DISABLE(uiIntFlag) ; */\
__asm__ __volatile__("movw %ss:(0), %ds") ; \
__asm__ __volatile__("movw %ss:(2), %fs") ; \
__asm__ __volatile__("movw %ss:(4), %gs") ; \
__asm__ __volatile__("movw %ss:(6), %es") ; \
/*SAFE_INT_ENABLE(uiIntFlag) ; */\
}

#define SAFE_INT_DISABLE(SYNC_FLAG) \
__asm__ __volatile__("pushf") ; \
__asm__ __volatile__("popl %0" : "=m"(SYNC_FLAG) : ) ; \
if((SYNC_FLAG & 0x0200)) PIC::Instance().DisableAllInterrupts() ; 

#define SAFE_INT_ENABLE(SYNC_FLAG) \
if((SYNC_FLAG & 0x0200)) PIC::Instance().EnableAllInterrupts() ;

#define AsmUtil_DECLARE_KERNEL_DS_BACK_VAR \
__volatile__ unsigned short AsmUtil_usDS = MemUtil_GetDS() ; \
__volatile__ unsigned short AsmUtil_usES = MemUtil_GetES() ; \
__volatile__ unsigned short AsmUtil_usFS = MemUtil_GetFS() ; \
__volatile__ unsigned short AsmUtil_usGS = MemUtil_GetGS() ;

#define AsmUtil_SET_KERNEL_DATA_SEGMENTS \
AsmUtil_DECLARE_KERNEL_DS_BACK_VAR \
MemUtil_SetDS(SYS_DATA_SELECTOR_DEFINED) ; \
MemUtil_SetES(SYS_LINEAR_SELECTOR_DEFINED) ; \
MemUtil_SetFS(SYS_DATA_SELECTOR_DEFINED) ; \
MemUtil_SetGS(SYS_DATA_SELECTOR_DEFINED) ; 

#define AsmUtil_NODEC_SET_KERNEL_DATA_SEGMENTS \
MemUtil_SetDS(SYS_DATA_SELECTOR_DEFINED) ; \
MemUtil_SetES(SYS_LINEAR_SELECTOR_DEFINED) ; \
MemUtil_SetFS(SYS_DATA_SELECTOR_DEFINED) ; \
MemUtil_SetGS(SYS_DATA_SELECTOR_DEFINED) ; 

#define AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS \
MemUtil_SetFS(AsmUtil_usFS) ; \
MemUtil_SetGS(AsmUtil_usGS) ; \
MemUtil_SetES(AsmUtil_usES) ; \
MemUtil_SetDS(AsmUtil_usDS) ;

#define __SET_DS_ES_TO_DATA \
	__volatile__ unsigned __uiDS = MemUtil_GetDS() ; \
	__volatile__ unsigned __uiES = MemUtil_GetES() ; \
	MemUtil_SetDS(SYS_DATA_SELECTOR_DEFINED) ; \
	MemUtil_SetES(SYS_DATA_SELECTOR_DEFINED) ;

#define __RESTOR_DS_ES \
	MemUtil_SetDS(__uiDS) ; \
	MemUtil_SetES(__uiES) ;

#endif
	
#define AsmUtil_UNLOAD_KERNEL_SEGS_ON_STACK() \
{ \
__volatile__ unsigned uiIntFlag ; \
SAFE_INT_DISABLE(uiIntFlag) ; \
__asm__ __volatile__("popw %es") ; \
__asm__ __volatile__("popw %gs") ; \
__asm__ __volatile__("popw %fs") ; \
__asm__ __volatile__("popw %ds") ; \
SAFE_INT_ENABLE(uiIntFlag) ; \
}

#define AsmUtil_LOAD_KERNEL_SEGS_ON_STACK() \
{ \
__volatile__ unsigned uiIntFlag ; \
SAFE_INT_DISABLE(uiIntFlag) ; \
__asm__ __volatile__("pushw %ds") ; \
__asm__ __volatile__("pushw %fs") ; \
__asm__ __volatile__("pushw %gs") ; \
__asm__ __volatile__("pushw %es") ; \
\
__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; \
__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; \
__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; \
__asm__ __volatile__("popw %ds") ; \
__asm__ __volatile__("popw %fs") ; \
__asm__ __volatile__("popw %gs") ; \
\
__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; \
__asm__ __volatile__("popw %es") ; \
SAFE_INT_ENABLE(uiIntFlag) ; \
}
