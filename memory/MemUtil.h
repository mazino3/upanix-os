/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
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
#pragma once

#include <Global.h>

# define CMOS_COMMAND_PORT 0x70
# define CMOS_RW_PORT 0x71

#define DB 1
#define DW 2
#define DD 4

#define POINTER(PointerType, PointerVariable, Offset) \
*((PointerType*)(PointerVariable + Offset))


unsigned short MemUtil_GetSS() ;
unsigned short MemUtil_GetDS() ;
unsigned short MemUtil_GetES() ;
unsigned short MemUtil_GetFS() ;
unsigned short MemUtil_GetGS() ;
unsigned MemUtil_GetSP() ;

void MemUtil_MoveByte(const unsigned short usSelector, const unsigned int uiOffset, const byte uiValue) ;

#define MemUtil_SetDS(/*unsigned short*/ usSelector) \
{\
__asm__ __volatile__("pushw %0" : : "rm"((unsigned short)usSelector)) ; \
__asm__ __volatile__("popw %ds") ; \
/*__asm__ __volatile__("movw %%ss:%0, %%ds" : : "m"(usSelector)); \*/\
}

#define MemUtil_SetES(/*unsigned short*/ usSelector) \
{\
__asm__ __volatile__("pushw %0" : : "rm"((unsigned short)usSelector)) ; \
__asm__ __volatile__("popw %es") ; \
/*__asm__ __volatile__("movw %%ss:%0, %%es" : : "m"(usSelector)); \*/\
}

#define MemUtil_SetFS(/*unsigned short*/ usSelector) \
{\
__asm__ __volatile__("pushw %0" : : "rm"((unsigned short)usSelector)) ; \
__asm__ __volatile__("popw %fs") ; \
/*__asm__ __volatile__("movw %%ss:%0, %%fs" : : "m"(usSelector)); \*/\
}

#define MemUtil_SetGS(/*unsigned short*/ usSelector) \
{\
__asm__ __volatile__("pushw %0" : : "rm"((unsigned short)usSelector)) ; \
__asm__ __volatile__("popw %gs") ; \
/*__asm__ __volatile__("movw %%ss:%0, %%gs" : : "m"(usSelector)); \*/\
}

void MemUtil_CopyMemory(__volatile__ unsigned short usSrcSelector, unsigned uiSrcBuffer,
			__volatile__ unsigned short usDestSelecttor, unsigned uiDestBuffer, unsigned uiNoOfBytes) ;
void MemUtil_CopyMemoryBack(__volatile__ unsigned short usSrcSelector, unsigned uiSrcBuffer,
	   		__volatile__ unsigned short usDestSelector, unsigned uiDestBuffer, unsigned uiNoOfBytes) ;
				
unsigned MemUtil_GetCMOS(const byte bAddr) ;
void MemUtil_PutCMOS(const byte bAddr, const byte bValue) ;
unsigned MemUtil_GetExtendedMemorySize() ;
void MemUtil_Set(byte* pBuffer, byte bVal, unsigned uiLen) ;